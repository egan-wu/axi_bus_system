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
    uint32_t m_arid = 0x00;
    uint32_t m_awid = 0x00;

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

    // AW channel
    sc_out<bool>      awvalid;   // master -> slave
    sc_in<bool>       awready;   // slave  -> master
    sc_out<uint32_t>  awid;      // master -> slave
    sc_out<uint32_t>  awaddr;    // master -> slave
    sc_out<uint32_t>  awsize;    // master -> slave
    sc_out<uint32_t>  awlen;     // master -> slave

    // W channel
    sc_in<bool>       wvalid;
    sc_out<bool>      wready;
    sc_in<uint32_t>   wid;
    sc_out<uint32_t>  wdata;

    // // B channel
    // sc_out<bool>     bvalid_m;
    // sc_in<bool>      bready_m;
    // sc_in<uint32_t>  bresp_m;

    sc_mutex fifo_mutex;
    std::deque<AXI_REQ> req_fifo;

    SC_CTOR(AXIMaster) {
        srand(time(0));

        SC_THREAD(gen_cmd_process);
        sensitive << clk.pos();

        SC_THREAD(ar_process);
        sensitive << clk.pos();

        SC_THREAD(r_process);
        sensitive << clk.pos();

        SC_THREAD(aw_process);
        sensitive << clk.pos();

        SC_THREAD(w_process);
        sensitive << clk.pos();

        arvalid.initialize(false);
        araddr.initialize(0);
    }

    void read (uint32_t addr) {
        AXI_REQ req;
        req.type = READ;
        req.addr = addr;
        fifo_mutex.lock();
        req_fifo.push_back(req);
        fifo_mutex.unlock();
    }

    void write (uint32_t addr) {
        AXI_REQ req;
        req.type = WRITE;
        req.addr = addr;
        fifo_mutex.lock();
        req_fifo.push_back(req);
        fifo_mutex.unlock();
    }

private:
    std::deque<uint32_t> aw_fifo;
    std::unordered_map<uint32_t, AXI_REQ> ar_requests;
    std::unordered_map<uint32_t, AXI_REQ> aw_requests;

    int randn(int min, int max) {
        int random_number = rand() % (max - min + 1) + min;
        return random_number;
    }

    bool req_queue_empty () {
        fifo_mutex.lock();
        bool is_empty = req_fifo.empty();
        fifo_mutex.unlock();
        return is_empty;
    }

    void gen_cmd_process() {
        uint32_t type;
        uint32_t row;
        uint32_t col;
        uint32_t addr;
        
        while (true) {
            type = randn(READ, WRITE);
            row  = randn(0, 0);
            col  = randn(0, 0);
            addr = ADDRESS(row, col);

            if (type == READ) {
                read(addr);
            } else {
                write(addr);
            }

            wait(2, sc_core::SC_NS);
        }
    }

    void ar_process () {
        while (true) {
            while (req_queue_empty() || req_fifo.front().type != READ) {
                wait();
            }

            wait();
            AXI_REQ ar_req = req_fifo.front();
            req_fifo.pop_front();
    
            {
                // for AR_REQ and insert ar_requests
                ar_req.id = m_arid++;
                ar_req.size = 0x7;
                ar_req.len = randn(0, 4); // BL2
                ar_requests.insert({ar_req.id, ar_req});
                // std::cout << "[Master][AR] send ar_req { arid: " << ar_req.arid << ", araddr: " << ar_req.araddr << " [r:" << ROW_INDEX(ar_req.araddr) << ",c:" << COL_INDEX(ar_req.araddr) << "] , arsize: " << ar_req.arsize << ", arlen: " << ar_req.arlen << "}" << std::endl;

                // send ar_request
                arid.write(ar_req.id);  
                araddr.write(ar_req.addr);
                arsize.write(ar_req.size);
                arlen.write(ar_req.len); 
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
                    AXI_REQ r_req = ar_requests[id];
                    uint32_t total_offset = ((1 << r_req.size) * (r_req.len + 1)) >> BUS_WIDTH;
                    for (uint32_t offset = 0; offset < total_offset; offset++) {
                        while (rvalid.read() == false) {
                            wait();
                        }
                        uint32_t read_data = rdata.read();
                        // std::cout << "[Master][id:" << id << "][offset:" << offset << "] " << std::hex << read_data << std::dec << std::endl;
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

    void aw_process () {
        while (true) {
            while (req_queue_empty() || req_fifo.front().type != WRITE) {
                wait();
            }

            wait();
            fifo_mutex.lock();
            AXI_REQ aw_req = req_fifo.front();
            fifo_mutex.unlock();
            req_fifo.pop_front();
    
            {
                // for AW_REQ and insert ar_requests
                aw_req.id = m_awid++;
                aw_req.size = 0x7;
                aw_req.len = randn(0, 4); // BL2
                aw_requests.insert({aw_req.id, aw_req});
                // std::cout << "[Master][AW] send aw_req { id: " << aw_req.id << ", addr: " << aw_req.addr << " [r:" << ROW_INDEX(aw_req.addr) << ",c:" << COL_INDEX(aw_req.addr) << "] , size: " << aw_req.size << ", len: " << aw_req.len << "}" << std::endl;

                // send ar_request
                awid.write(aw_req.id);  
                awaddr.write(aw_req.addr);
                awsize.write(aw_req.size);
                awlen.write(aw_req.len); 
                awvalid.write(true);
                wait();
            }

            while (awready.read() == false) {
                wait();
            }

            awvalid.write(false);
        }
    }

    void w_process () {
        while (true) {
            wait();

            while (wvalid.read() == false) {
                wait();
            }

            uint32_t id = wid.read();
            AXI_REQ w_req = aw_requests[id];
            uint32_t total_offset = ((1 << w_req.size) * (w_req.len + 1)) >> BUS_WIDTH;

            wready.write(true);
            for (uint32_t offset = 0; offset < total_offset; offset++) {
                while (wvalid.read() == false) {
                    wait();
                }
                uint32_t write_data = randn(0, 0xFFFF);
                wdata.write(write_data);
                // std::cout << "[Master][id:" << id << "][offset:" << offset << "] " << std::hex << write_data << std::dec << std::endl;
                wait();
            }

            wready.write(false);

            while (wvalid.read() == true) {
                wait();
            }
            wait();
        }
    }
};