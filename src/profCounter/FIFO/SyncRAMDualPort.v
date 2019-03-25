`timescale 1ns / 1ps

module SyncRAMDualPort#(
	parameter ADDR_WIDTH = 16,
	parameter DATA_WIDTH = 32
) (
	/* Standard pins */
	clk,

	/* Port A */
	addressA,
	writeA,
	writeDataA,
	readDataA,
	/* Port B */
	addressB,
	writeB,
	writeDataB,
	readDataB
);

	input clk;

	input [ADDR_WIDTH-1:0] addressA;
	input writeA;
	input [DATA_WIDTH-1:0] writeDataA;
	output [DATA_WIDTH-1:0] readDataA;
	input [ADDR_WIDTH-1:0] addressB;
	input writeB;
	input [DATA_WIDTH-1:0] writeDataB;
	output [DATA_WIDTH-1:0] readDataB;

	reg [DATA_WIDTH-1:0] mem [0:(1 << ADDR_WIDTH)-1];
	//reg [DATA_WIDTH-1:0] intReadDataA;
	//reg [DATA_WIDTH-1:0] intReadDataB;

	// XXX: is this inferrable?
	assign readDataA = mem[addressA];
	assign readDataB = mem[addressB];

	always @(posedge clk) begin
		//intReadDataA <= mem[addressA];

		if(writeA) begin
			mem[addressA] <= writeDataA;
			//intReadDataA <= writeDataA;
		end
	end

	always @(posedge clk) begin
		//intReadDataB <= mem[addressB];

		if(writeB) begin
			mem[addressB] <= writeDataB;
			//intReadDataB <= writeDataB;
		end
	end

endmodule
