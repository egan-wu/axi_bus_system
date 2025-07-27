#pragma once
#include <systemc>
#include <deque>
#include <unordered_map>
#include <cstdlib>
#include <ctime>
#include "AXICommon.hpp"

using namespace sc_core;

SC_MODULE (AXIMaster) {
    double total_data_received = 0;

    sc_in<bool>      clk;
        
    // AR channel
    sc_out<bool>      arvalid;   // master -> slave
    sc_in<bool>       arready;   // slave  -> master
    sc_out<uint32_t>  arid;      // master -> slave
    sc_out<uint32_t>  araddr;    // master -> slave
    sc_out<uint32_t>  arsize;    // master -> slave
    sc_out<uint32_t>  arlen;     // master -> slave

    // R channel
    sc_in<bool>       rvalid;
    sc_out<bool>      rready;
    sc_in<uint32_t>   rid;
    sc_in<uint32_t>   rdata;

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

    sc_mutex fifo_mutex;
    std::deque<uint32_t> read_fifo;

    SC_CTOR(AXIMaster) {
        srand(time(0));

        SC_THREAD(ar_process);
        sensitive << clk.pos();

        SC_THREAD(gen_cmd_process);
        sensitive << clk.pos();

        SC_THREAD(r_process);
        sensitive << clk.pos();

        arvalid.initialize(false);
        araddr.initialize(0);
    }

    void read (uint32_t addr) {
        fifo_mutex.lock();
        read_fifo.push_back(addr);
        fifo_mutex.unlock();
    }

private:

    uint32_t m_arid = 0x00;
    std::unordered_map<uint32_t, AR_REQ> ar_requests;

    int randn(int min, int max) {
        int random_number = rand() % (max - min + 1) + min;
        return random_number;
    }

    void gen_cmd_process() {
        uint32_t row;
        uint32_t col;
        uint32_t addr;
        // std::cout << ROW_INDEX(addr) << "," << COL_INDEX(addr) << ":" << addr << std::endl;
        
        while (true) {
            wait(2, sc_core::SC_NS);

            row = randn(0, 0);
            col = randn(0, 0);
            addr = ADDRESS(row, col);
            read(addr);
        }
    }

    void ar_process () {
        while (true) {
            while (read_fifo.empty()) {
                wait();
            }

            wait();
            uint32_t addr = read_fifo.front();
            read_fifo.pop_front();
    
            {
                // for AR_REQ and insert ar_requests
                AR_REQ ar_req;
                ar_req.arid = m_arid++;
                ar_req.araddr = addr;
                ar_req.arsize = 0x7;
                ar_req.arlen = randn(0, 4); // BL2
                ar_requests.insert({ar_req.arid, ar_req});
                // std::cout << "[Master][AR] send ar_req { arid: " << ar_req.arid << ", araddr: " << ar_req.araddr << " [r:" << ROW_INDEX(ar_req.araddr) << ",c:" << COL_INDEX(ar_req.araddr) << "] , arsize: " << ar_req.arsize << ", arlen: " << ar_req.arlen << "}" << std::endl;

                // send ar_request
                arid.write(ar_req.arid);  
                araddr.write(ar_req.araddr);
                arsize.write(ar_req.arsize);
                arlen.write(ar_req.arlen); 
                arvalid.write(true);
                wait();
            }

            while (arready.read() == false) {
                wait();
            }

            arvalid.write(false);
        }
    }

    void r_process () {
        while (true) {
            wait();
            while (ar_requests.empty()) {
                wait();
            }

            // listen to rvalid, wait for rdata
            while (rvalid.read() == false) {
                wait();
            }

            {
                // assert rready to ack slave
                rready.write(true);
                wait();

                while (rvalid.read() == false) {
                    wait();
                }
                uint32_t id = rid.read();
                wait();

                if (ar_requests.find(id) != ar_requests.end()) {
                    AR_REQ r_req = ar_requests[id];
                    std::cout << "[Master][AR] send ar_req { rid: " << r_req.arid << ", raddr: " << r_req.araddr << " [r:" << ROW_INDEX(r_req.araddr) << ",c:" << COL_INDEX(r_req.araddr) << "] , rsize: " << r_req.arsize << ", rlen: " << r_req.arlen << "}" << std::endl;
                    uint32_t total_offset = ((1 << r_req.arsize) * (r_req.arlen + 1)) >> BUS_WIDTH;
                    for (uint32_t offset = 0; offset < total_offset; offset++) {
                        while (rvalid.read() == false) {
                            wait();
                        }
                        uint32_t read_data = rdata.read();
                        std::cout << "[Master][id:" << id << "][offset:" << offset << "] " << std::hex << read_data << std::dec << std::endl;
                        total_data_received += (1 << BUS_WIDTH);
                        wait();
                    }
                    ar_requests.erase(id);
                } 
                else { assert(0); }

                rready.write(false);
                wait();

                // wait until slave deassert rvalid
                while (rvalid.read() == true) {
                    wait();
                }
            }
        }
    }

};