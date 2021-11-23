// N - Length of the buffer
// M - Width of the register

module fifo #(parameter N = 16, parameter M = 32)
(
	output [M-1:0] data_out,													// Data output
	output fe, ff, fo,															// Status outputs
	input [M-1:0] data_in,														// Data input
	input clk, reset, chipselect, read, write, ov_clear,				// Control signals
	
	output [$clog2(N)-1:0] rp_debug_out, wp_debug_out					// Used for debugging
);
	
	reg [M-1:0] buffer [N - 1:0];
	reg [$clog2(N)-1:0] rp, wp;												// Read Pointer, Write Pointer
	reg [$clog2(N):0] pd;
	reg ov;
	
	assign fe = (pd == 1'b0);
	assign ff = (pd == N);
	assign fo = ov;
	assign rp_debug_out = rp;
	assign wp_debug_out = wp;
	assign data_out = buffer[rp];
	
	// Overflow Block
	always @ (posedge clk)
	begin
		if(reset)
			ov <= 1'b0;
		// Bit 3 represents txfo w1c
		else if(ov_clear)
			ov <= 1'b0;
		else
		begin
			if(chipselect && write)
			begin
				// If the FIFO is full, and we are writing to the FIFO (Data Reg)
				// and if the overflow is not set, write a 1 to it.
				if(ff && !ov)
					ov <= 1'b1;
				// If none of the above conditions are true, don't change
				// the value of the overflow flag.
			end
		end
	end
	
	// Fifo Block
	always @ (posedge clk)
	begin
		if(reset)
		begin
			rp <= 0;
			wp <= 0;
			pd <= 0;
		end
		else
		begin
			if(chipselect && read && !fe)
			begin
				// data_out <= buffer[rp];
				rp <= rp + 1'b1;
				pd <= pd - 1'b1;
			end
		else if(chipselect && write && !ff && !ov)
			begin
				buffer[wp] <= data_in;
				wp <= wp + 1'b1;
				pd <= pd + 1'b1;
			end
		end
	end

endmodule
