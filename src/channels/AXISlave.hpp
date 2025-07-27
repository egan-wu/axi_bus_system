#pragma once
#include <systemc>
#include <deque>
#include <string>
#include <iostream>
#include <unordered_map>
#include "AXICommon.hpp"

using namespace sc_core;

SC_MODULE (AXISlave) {
    sc_in<bool>      clk;
        
    // AR channel
    sc_in<bool>       arvalid;   // master -> slave
    sc_out<bool>      arready;   // slave  -> master
    sc_in<uint32_t>   arid;      // master -> slave
    sc_in<uint32_t>   araddr;    // master -> slave
    sc_in<uint32_t>   arsize;    // master -> slave
    sc_in<uint32_t>   arlen;     // master -> slave

    // R channel
    sc_out<bool>      rvalid;    // slave  -> master
    sc_in<bool>       rready;    // master -> slave
    sc_out<uint32_t>  rid;       // slave  -> master
    sc_out<uint32_t>  rdata;     // slave  -> master

    // // AW channel
    // sc_in<bool>      awvalid;
    // sc_out<bool>     awready;
    // sc_in<uint32_t>  awaddr;

    // // W channel
    // sc_in<bool>      wvalid;
    // sc_out<bool>     wready;
    // sc_in<uint32_t>  wdata;

    // // B channel
    // sc_out<bool>     bvalid_m;
    // sc_in<bool>      bready_m;
    // sc_in<uint32_t>  bresp_m;

    SC_CTOR(AXISlave) {
        SC_THREAD(ar_process);
        sensitive << clk.pos();

        SC_THREAD(r_process);
        sensitive << clk.pos();

        arready.initialize(false);
        rvalid.initialize(false);

        dram.resize(16, std::vector<uint32_t>(4096, 0x0));
        int data = 0;
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 4096; j++) {
                dram[i][j] = data++;
            }
        }
    }

private:

    // ar channel parameters
    std::deque<uint32_t> ar_fifo;
    std::unordered_map<uint32_t, AR_REQ> ar_requests;
    std::vector<std::vector<uint32_t>> dram;
    uint32_t curr_row = 0;

    void ar_process () {
        while (true) {
            wait();
            while (arvalid.read() == false) {
                wait();
            }

            {
                // read ar_request and insert AR request map
                uint32_t addr = araddr.read();
                uint32_t size = arsize.read();
                uint32_t len = arlen.read();
                uint32_t id = arid.read();
                AR_REQ ar_req;
                ar_req.arid = id;
                ar_req.araddr = addr;
                ar_req.arsize = size;
                ar_req.arlen = len;
                ar_requests.insert({id, ar_req});
                ar_fifo.push_back(id);
                // std::cout << "[Slave ][AR] recv ar_req { arid: " << id << ", araddr: " << addr <<  ", arsize: " << size << ", arlen: " << len << "}" << std::endl;
            }
            arready.write(true);
            wait();

            while (arvalid.read() == false) {
                wait();
            }

            // wait();
            arready.write(false);
        }
    }

    void r_process () {
        while (true) {
            wait();
            while (ar_fifo.empty()) {
                wait();
            }

            {
                // read data from dram and send through rdata
                uint32_t id = ar_fifo.front();
                if (ar_requests.find(id) != ar_requests.end()) {

                    AR_REQ ar_req = ar_requests[id];
                    uint32_t row = ROW_INDEX(ar_req.araddr);
                    uint32_t col = COL_INDEX(ar_req.araddr);

                    {
                        if (curr_row != row) {
                            wait(500, sc_core::SC_NS);
                        }
                        curr_row = row;
                    }

                    rid.write(ar_req.arid);
                    rvalid.write(true);
                    wait();
                    
                    // address range check
                    if (row >= dram.size() || col + ar_req.arsize * ar_req.arlen >= dram[0].size()) {
                        std::cout << "row: " << row << ", col: " << col << std::endl;
                        SC_REPORT_ERROR("AXISlave", "Address out of bounds!");
                        sc_core::sc_stop();
                        return;
                    }

                    uint32_t total_offset = ((1 << ar_req.arsize) * (ar_req.arlen + 1)) >> BUS_WIDTH;
                    for (uint32_t offset = 0; offset < total_offset; offset++) {
                        while (rready.read() == false) {
                            wait();
                        }
                        rdata.write(dram[row][col + offset]);
                        wait();
                    }

                    {
                        // AR request done, remove from list
                        ar_fifo.pop_front();
                        ar_requests.erase(id);
                    }

                    rvalid.write(false);
                    wait();

                    // wait until master deassert rready
                    while (rready.read() == false) {
                        wait();
                    }
                } 
                else { assert(0); }
            }
        }
    }

};