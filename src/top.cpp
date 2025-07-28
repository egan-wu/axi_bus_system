#include <systemc>
#include <iostream>
#include "config.hpp"

#include "channels/AXIMaster.hpp"
#include "channels/AXISlave.hpp"
#include "config.hpp"

int sc_main(int argc, char* argv[]) {
    std::cout << "Starting simulation for project: practice07_bus_system" << std::endl;

    config_loader m_config_loader;
    m_config_loader.load_yaml();

    AXIMaster master_inst("master_instance");
    AXISlave slave_inst("slave_instance");

    sc_core::sc_trace_file* tf = sc_core::sc_create_vcd_trace_file("axi_ar_waveform");
    if (!tf) {
        std::cerr << "Error: Could not create trace file!" << std::endl;
        return 1;
    }

    sc_core::sc_clock clk("main_clock", m_config_loader.cfg.clock.period_ns, sc_core::SC_NS, 0.5, 5, sc_core::SC_NS, true);
    master_inst.clk(clk);
    slave_inst.clk(clk);
    sc_core::sc_trace(tf, clk, "clk");

    // AR channel
    sc_core::sc_signal<bool> arvalid_signal("arvalid_signal");
    sc_core::sc_signal<bool> arready_signal("arready_signal");
    sc_core::sc_signal<uint32_t> arid_signal("arid_signal");
    sc_core::sc_signal<uint32_t> araddr_signal("araddr_signal");
    sc_core::sc_signal<uint32_t> arsize_signal("arsize_signal");
    sc_core::sc_signal<uint32_t> arlen_signal("arlen_signal");
    master_inst.arvalid(arvalid_signal);
    slave_inst.arvalid(arvalid_signal);
    master_inst.arready(arready_signal);
    slave_inst.arready(arready_signal);
    master_inst.arid(arid_signal);
    slave_inst.arid(arid_signal);
    master_inst.araddr(araddr_signal);
    slave_inst.araddr(araddr_signal);
    master_inst.arsize(arsize_signal);
    slave_inst.arsize(arsize_signal);
    master_inst.arlen(arlen_signal);
    slave_inst.arlen(arlen_signal);
    sc_core::sc_trace(tf, master_inst.arvalid, "arvalid");
    sc_core::sc_trace(tf, master_inst.arready, "arready");
    sc_core::sc_trace(tf, master_inst.arid, "arid");
    sc_core::sc_trace(tf, master_inst.araddr, "araddr");
    sc_core::sc_trace(tf, master_inst.arsize, "arsize");
    sc_core::sc_trace(tf, master_inst.arlen, "arlen");

    // R channel
    sc_core::sc_signal<bool> rvalid_signal("rvalid_signal");
    sc_core::sc_signal<bool> rready_signal("rready_signal");
    sc_core::sc_signal<uint32_t> rid_signal("rid_signal");
    sc_core::sc_signal<uint32_t> rdata_signal("rdata_signal");
    master_inst.rvalid(rvalid_signal);
    slave_inst.rvalid(rvalid_signal);
    master_inst.rready(rready_signal);
    slave_inst.rready(rready_signal);
    master_inst.rid(rid_signal);
    slave_inst.rid(rid_signal);
    master_inst.rdata(rdata_signal);
    slave_inst.rdata(rdata_signal);
    sc_core::sc_trace(tf, master_inst.rvalid, "rvalid");
    sc_core::sc_trace(tf, master_inst.rready, "rready");
    sc_core::sc_trace(tf, master_inst.rid, "rid");
    sc_core::sc_trace(tf, master_inst.rdata, "rdata");

    // AW channel
    sc_core::sc_signal<bool> awvalid_signal("awvalid_signal");
    sc_core::sc_signal<bool> awready_signal("awready_signal");
    sc_core::sc_signal<uint32_t> awid_signal("awid_signal");
    sc_core::sc_signal<uint32_t> awaddr_signal("awaddr_signal");
    sc_core::sc_signal<uint32_t> awsize_signal("awsize_signal");
    sc_core::sc_signal<uint32_t> awlen_signal("awlen_signal");
    master_inst.awvalid(awvalid_signal);
    slave_inst.awvalid(awvalid_signal);
    master_inst.awready(awready_signal);
    slave_inst.awready(awready_signal);
    master_inst.awid(awid_signal);
    slave_inst.awid(awid_signal);
    master_inst.awaddr(awaddr_signal);
    slave_inst.awaddr(awaddr_signal);
    master_inst.awsize(awsize_signal);
    slave_inst.awsize(awsize_signal);
    master_inst.awlen(awlen_signal);
    slave_inst.awlen(awlen_signal);
    sc_core::sc_trace(tf, master_inst.awvalid, "awvalid");
    sc_core::sc_trace(tf, master_inst.awready, "awready");
    sc_core::sc_trace(tf, master_inst.awid, "awid");
    sc_core::sc_trace(tf, master_inst.awaddr, "awaddr");
    sc_core::sc_trace(tf, master_inst.awsize, "awsize");
    sc_core::sc_trace(tf, master_inst.awlen, "awlen");

    // W channel
    sc_core::sc_signal<bool> wvalid_signal("wvalid_signal");
    sc_core::sc_signal<bool> wready_signal("wready_signal");
    sc_core::sc_signal<uint32_t> wid_signal("wid_signal");
    sc_core::sc_signal<uint32_t> wdata_signal("wdata_signal");
    master_inst.wvalid(wvalid_signal);
    slave_inst.wvalid(wvalid_signal);
    master_inst.wready(wready_signal);
    slave_inst.wready(wready_signal);
    master_inst.wid(wid_signal);
    slave_inst.wid(wid_signal);
    master_inst.wdata(wdata_signal);
    slave_inst.wdata(wdata_signal);
    sc_core::sc_trace(tf, master_inst.wvalid, "wvalid");
    sc_core::sc_trace(tf, master_inst.wready, "wready");
    sc_core::sc_trace(tf, master_inst.wid, "wid");
    sc_core::sc_trace(tf, master_inst.wdata, "wdata");

    double exe_time = 500000;
    sc_core::sc_start(exe_time, sc_core::SC_NS);

    std::cout << "total_data_received: " << master_inst.total_data_received << " bytes" << std::endl;
    std::cout << "throughput: " << (((master_inst.total_data_received / 1000000000)) / (exe_time * 0.000000001)) << " GB/s" << std::endl;
    std::cout << "total_data_written: " << slave_inst.total_data_written << " bytes" << std::endl;
    std::cout << "throughput: " << (((slave_inst.total_data_written / 1000000000)) / (exe_time * 0.000000001)) << " GB/s" << std::endl;
    std::cout << "Simulation for project: practice07_bus_system finished." << std::endl;
    return 0;
}
