#pragma once
#include <iostream>
#include <vector>
#include <deque>
#include <systemc>
#include "AXICommon.hpp"

using namespace sc_core;

struct DDR_CMD {
    uint32_t type;
    uint32_t address;
    uint32_t burst;

    bool operator==(const DDR_CMD& other) const {
        return (type == other.type && address == other.address && burst == other.burst);
    }
};

inline std::ostream& operator<<(std::ostream& os, const DDR_CMD& d) {
    os << "{type: " << ((d.type == READ) ? "READ" : "WRITE") << ", addr: " << std::hex << d.address << ", burst: " << d.burst << "}" << std::dec;
    return os;
}

inline void sc_trace(sc_core::sc_trace_file* tf, const DDR_CMD& cmd, const std::string& name) {
    sc_trace(tf, cmd.type, name + ".type");
    sc_trace(tf, cmd.address, name + ".address");
    sc_trace(tf, cmd.burst, name + ".burst");
}

SC_MODULE (DDR) {
    sc_in<bool> clk;
    sc_in<DDR_CMD> ca;
    sc_in<bool> ca_en;
    sc_out<bool> data_ready;
    sc_out<uint32_t> data_out;
    sc_in<uint32_t> data_in;
    std::deque<DDR_CMD> cmd_fifo;

    SC_CTOR(DDR) {
        ddr.resize(BANK_NUM);
        for (auto &bank : ddr) {
            bank.resize(ROW_NUM);
            for (auto &row : bank) {
                row.resize(COL_NUM, 0xDEAD0000);
            }
        }

        uint32_t val;
        for (size_t i = 0; i < ddr.size(); i++) {
            for (size_t j = 0; j < ddr[i].size(); j++) {
                val = 0;
                for (size_t k = 0; k < ddr[i][j].size(); k++) {
                    ddr[i][j][k] = 0xDEAD0000 + (val++);
                }
            }
        }

        curr_type = 0;
        curr_open_row.resize(1 << BANK_BIT, -1);

        SC_THREAD(cmd_process);
        sensitive << clk.pos();

        SC_THREAD(data_process);
        sensitive << clk.pos();
    }

private:
    std::vector<std::vector<std::vector<uint32_t>>> ddr;
    sc_mutex fifo_mutex;
    sc_event fifo_pop_event;
    sc_event data_ready_event;

    uint32_t curr_type;
    std::vector<uint32_t> curr_open_row;

    uint32_t fifo_remain() {
        fifo_mutex.lock();
        uint32_t cnt = cmd_fifo.size();
        fifo_mutex.unlock();
        return cnt;
    }

    void push_cmd_fifo (DDR_CMD cmd) {
        fifo_mutex.lock();
        cmd_fifo.push_back(cmd);
        fifo_mutex.unlock();
    }

    void pop_cmd_fifo () {
        fifo_mutex.lock();
        cmd_fifo.pop_front();
        fifo_mutex.unlock();
        fifo_pop_event.notify();
    }

    bool cmd_fifo_empty() {
        fifo_mutex.lock();
        bool empty = cmd_fifo.empty();
        fifo_mutex.unlock();
        return empty;
    }

    DDR_CMD peak_cmd_fifo () {
        fifo_mutex.lock();
        DDR_CMD cmd = cmd_fifo.front();
        fifo_mutex.unlock();
        return cmd;
    }

    void cmd_process () {
        while (true) {
            while (ca_en.read() == false) {
                wait();
            }

            DDR_CMD cmd = ca.read();
            push_cmd_fifo(cmd);

            wait(data_ready_event);
            wait();
        }
    }

    void data_process () {
        while (true) {
            while (cmd_fifo_empty() == true) {
                wait();
            }

            DDR_CMD cmd = peak_cmd_fifo();
            if (ROW(cmd.address) != curr_open_row[BANK(cmd.address)]) {
                wait(50, sc_core::SC_NS); // switch row
                curr_open_row[BANK(cmd.address)] = ROW(cmd.address);
            }

            data_ready.write(true);
            wait();
            data_ready_event.notify();

            wait();
            if (cmd.type == READ) {
                for (uint32_t i = 0; i < cmd.burst; i++) {
                    data_out.write(ddr[BANK(cmd.address)][ROW(cmd.address)][COL(cmd.address) + i]);
                    // std::cout << "data: " << std::hex << ddr[BANK(cmd.address)][ROW(cmd.address)][COL(cmd.address) + i] << std::endl;
                    wait();
                }
                pop_cmd_fifo();
            }
            else if (cmd.type == WRITE) {
                
                pop_cmd_fifo();
            }
            else {
                assert(0);
            }
            data_ready.write(false);
            wait();
        }
    }
};