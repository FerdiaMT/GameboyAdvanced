`timescale 1ns/1ps

// Host-side harness for chsasank/ARM7.  It intentionally tests only the
// reference's implemented ARM data-processing path: no memory, branches, PC
// operands, or Thumb state.  State is injected hierarchically because the
// upstream educational RTL has no reset/debug interface.
module arm7_reference_tb;
    reg clk = 0;
    reg [1023:0] program_file;
    reg [1023:0] state_file;
    reg [31:0] initial_registers [0:14];
    integer index;

    DeepPipeline dut(.clk(clk));

    task tick;
        begin
            #5 clk = 1;
            #5 clk = 0;
        end
    endtask

    task initialise_pipeline;
        begin
            dut.fetch_instr = 0;
            dut.pc_update = 0;
            dut.F_R_instr = 32'he1a00000;
            dut.F_R_type = 9;
            dut.F_R_flush = 0;
            dut.R_MUL_type = 9;
            dut.R_MUL_instr = 32'he1a00000;
            dut.R_MUL_out_data_1 = 0;
            dut.R_MUL_out_data_2 = 0;
            dut.R_MUL_out_data_3 = 0;
            dut.R_MUL_out_data_4 = 0;
            dut.R_MUL_flush = 0;
            dut.MUL_ALU_type = 9;
            dut.MUL_ALU_instr = 32'he1a00000;
            dut.MUL_ALU_out_data_1 = 0;
            dut.MUL_ALU_out_data_2 = 0;
            dut.MUL_ALU_out_data_3 = 0;
            dut.MUL_ALU_out_data_4 = 0;
            dut.MUL_ALU_mult_result = 0;
            dut.MUL_ALU_will_this_be_executed = 0;
            dut.MUL_ALU_flush = 0;
            dut.ALU_MEM_will_this_be_executed = 0;
            dut.ALU_MEM_type = 9;
            dut.ALU_MEM_instr = 32'he1a00000;
            dut.ALU_MEM_out_data_1 = 0;
            dut.ALU_MEM_out_data_2 = 0;
            dut.ALU_MEM_out_data_3 = 0;
            dut.ALU_MEM_out_data_4 = 0;
            dut.ALU_MEM_mult_result = 0;
            dut.ALU_MEM_alu_result = 0;
            dut.ALU_MEM_alu_nzcv = 0;
            dut.ALU_MEM_alu_is_writeback = 0;
            dut.ALU_MEM_flush = 0;
            dut.MEM_W_will_this_be_executed = 0;
            dut.MEM_W_type = 9;
            dut.MEM_W_instr = 32'he1a00000;
            dut.MEM_W_out_data_1 = 0;
            dut.MEM_W_out_data_2 = 0;
            dut.MEM_W_out_data_3 = 0;
            dut.MEM_W_out_data_4 = 0;
            dut.MEM_W_memory_out_data = 0;
            dut.MEM_W_mult_result = 0;
            dut.MEM_W_alu_result = 0;
            dut.MEM_W_alu_nzcv = 0;
            dut.MEM_W_alu_is_writeback = 0;
            dut.MEM_W_flush = 0;
            dut.want_to_flush = 0;
            dut.number_stalls = 0;
        end
    endtask

    initial begin
        if (!$value$plusargs("program=%s", program_file)
            || !$value$plusargs("state=%s", state_file)) begin
            $display("Missing +program=<path> or +state=<path>");
            $finish;
        end

        // Let upstream initial blocks settle before replacing their fixed
        // Fibonacci program and zeroed register state.
        #1;
        $readmemh(program_file, dut.InstructionCache.M);
        $readmemh(state_file, initial_registers);
        for (index = 0; index < 15; index = index + 1)
            dut.RegisterFile.R[index] = initial_registers[index];
        dut.RegisterFile.R[15] = 0;
        dut.RegisterFile.cspr = 0;
        dut.current_pc = 0;
        initialise_pipeline();

        // Ten instructions plus NOP padding need substantially fewer than 64
        // cycles, but this also drains the six-stage pipeline deterministically.
        for (index = 0; index < 64; index = index + 1)
            tick();

        $write("ARM7_REF_STATE");
        for (index = 0; index < 15; index = index + 1)
            $write(" r%0d=%08x", index, dut.RegisterFile.R[index]);
        $write("\n");
        $finish;
    end
endmodule
