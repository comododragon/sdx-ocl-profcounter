`timescale 1ns / 1ps

/**
 * CommandUnit
 *
 * This module receive commands from the OpenCL pipe p0 and generates commands for other modules inside this RTL kernel.
 *
 * Command | Description
 *     0x0 | NOP
 *     0x1 | Save timestamp
 *     0x2 | Finish kernel execution
 */
module CommandUnit(
	/* Standard pins */
	clk,
	rst_n,

	/* Starts command unit */
	start,
	/* When a 0x2 command is received through the pipe, the module finishes execution and asserts done when finished */
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
				/* 0x2 command received, stop kernel */
				if(pipeTVALID && 'h2 == pipeTDATA[3:0]) begin
					state <= 'h0;
				end
			end
			else begin
				state <= 'h0;
			end
		end
	end

/*
	// XXX: DUMMY LOGIC, IT GENERATES 10 WRITE SIGNALS EVERY 40 CLOCKS, THEN IT GENERATES A STOP SIGNAL

	reg [3:0] state;
	reg [31:0] counter;

	assign done = 'h0 == state;
	assign command = ('h1 == state)? (('h190 == counter)? 'h2 : (('h000 == (counter % 'h028))? 'h1 : 'h0)) : 'h0;

	always @(posedge clk) begin
		if(!rst_n) begin
			counter <= 'h000;
			state <= 'h0;
		end
		else begin
			if('h0 == state) begin
				if(start) begin
					counter <= 'h000;
					state <= 'h1;
				end
			end
			else if('h1 == state) begin
				counter <= counter + 'h001;

				if('h190 == counter)
					state <= 'h0;
			end
			else begin
				state <= 'h0;
			end
		end
	end
*/

endmodule
