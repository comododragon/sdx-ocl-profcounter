`timescale 1ns / 1ps

`include "commands.vh"

/**
 * CommandUnit
 *
 * This module receive commands from the OpenCL pipe p0 and generates commands for other modules inside this RTL kernel.
 *
 * Command     | Description
 * COMM_NOP    | NOP
 * COMM_STAMP  | Save timestamp
 * COMM_HOLD   | Hold: timestamp values are only written when 0x3 is issued (e.g. to avoid competition on global memory)
 * COMM_FINISH | Finish kernel execution
 */
module CommandUnit(
	/* Standard pins */
	clk,
	rst_n,

	/* Starts command unit */
	start,
	/* When a COMM_FINISH command is received through the pipe, the module finishes execution and asserts done when finished */
	done,

	/* AXI4-Stream pipe sink */
	pipeTDATA,
	pipeTVALID,
	pipeTREADY,

	/* Generated command */
	command
);

	input clk;
	input rst_n;

	input start;
	output done;

	input [31:0] pipeTDATA;
	input pipeTVALID;
	output pipeTREADY;

	output [3:0] command;

	reg [3:0] state;

	assign done = 'h0 == state;
	/* This module is always ready to receive pipe commands (as long as the kernel is running) */
	assign pipeTREADY = !done;
	/* Command is only generated when kernel is running and value from pipe is valid */
	assign command = ('h1 == state && pipeTVALID)? pipeTDATA[3:0] : 'h0;

	/* Main FSM */
	always @(posedge clk) begin
		if(!rst_n) begin
			state <= 'h0;
		end
		else begin
			/* State 0x0: kernel is idle */
			if('h0 == state) begin
				if(start) begin
					state <= 'h1;
				end
			end
			/* State 0x1: kernel is running and ready to receive orders */
			else if('h1 == state) begin
				/* COMM_FINISH command received, stop kernel */
				if(pipeTVALID && `COMM_FINISH == pipeTDATA[3:0]) begin
					state <= 'h0;
				end
			end
			else begin
				state <= 'h0;
			end
		end
	end

endmodule
