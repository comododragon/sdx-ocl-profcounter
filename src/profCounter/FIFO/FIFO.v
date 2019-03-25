`define CLOG2(x) (\
	(x <= 2)? 1 :\
	(x <= 4)? 2 :\
	(x <= 8)? 3 :\
	(x <= 16)? 4 :\
	(x <= 32)? 5 :\
	(x <= 64)? 6 :\
	(x <= 128)? 7 :\
	(x <= 256)? 8 :\
	(x <= 512)? 9 :\
	(x <= 1024)? 10 :\
	-1\
)

module FIFO#(
	parameter SIZE = 16,
	parameter DATA_WIDTH = 32
) (
	/* Standard pins */
	clk,
	rst_n,

	/* FIFO commands */
	enqueue,
	dequeue,
	/* FIFO input */
	back,
	/* FIFO output */
	front,
	/* Status signals */
	full,
	empty
);

	input clk;
	input rst_n;

	input enqueue;
	input dequeue;
	input [DATA_WIDTH-1:0] back;
	output [DATA_WIDTH-1:0] front;
	output full;
	output empty;

	reg [`CLOG2(SIZE)-1:0] backPointer;
	reg [`CLOG2(SIZE)-1:0] frontPointer;
	reg [`CLOG2(SIZE):0] occupied;
	wire [DATA_WIDTH-1:0] ramReadDataB;
	wire commitEnqueue;
	wire commitDequeue;

	assign front = ramReadDataB;
	assign full = SIZE == occupied;
	assign empty = 'h0 == occupied;

	/* Enqueue only happens when FIFO is not full OR when simultaneous enqueue/dequeue happens */
	assign commitEnqueue = (enqueue && !full) || (enqueue && dequeue);
	/* Dequeue only happens when FIFO is not empty OR when simultaneous enqueue/dequeue happens */
	assign commitDequeue = (dequeue && !empty) || (dequeue && enqueue);

	/* Dual-port asynchronous memory */
	SyncRAMDualPort#(`CLOG2(SIZE), DATA_WIDTH) ram(
		.clk(clk),

		/* Port A is FIFO input */
		.addressA(backPointer),
		.writeA(commitEnqueue),
		.writeDataA(back),
		.readDataA(),

		/* Port B is FIFO output */
		.addressB(frontPointer),
		.writeB(1'b0),
		.writeDataB(),
		.readDataB(ramReadDataB)
	);

	always @(posedge clk) begin
		if(!rst_n) begin
			backPointer <= 'h0;
			frontPointer <= 'h0;
			occupied <= 'h0;
		end
		else begin
			/* Update back pointer if enqueue is accepted */
			if(commitEnqueue)
				backPointer <= ((SIZE - 'h1) == backPointer)? 'h0 : (backPointer + 'h1);

			/* Update front pointer if dequeue is accepted */
			if(commitDequeue)
				frontPointer <= ((SIZE - 'h1) == frontPointer)? 'h0 : (frontPointer + 'h1);

			/* Occupied does not change if nothing happens or if simultaneous enqueue/dequeue happens */
			if(enqueue && !dequeue && !full)
				occupied <= occupied + 'h1;
			else if(!enqueue && dequeue && !empty)
				occupied <= occupied - 'h1;
		end
	end

endmodule
