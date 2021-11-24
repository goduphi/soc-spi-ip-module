// Detects either a positive or negative edge
module edge_detect
(
	input clk, reset, signal_in, positive_edge,
	output wire pulse_out
);
	// One pulse generation Block
	reg old_signal_in;
	always @ (posedge clk)
	begin
		if(reset)
			old_signal_in <= 1'b0;
		else
			// Store the 1 clock delayed write value into a register
			// We only want this signal to be assert for 1 clock
			// So, auto clear it
			old_signal_in <= signal_in;
	end
	
	assign pulse_out = (positive_edge) ? signal_in & ~old_signal_in : ~signal_in & old_signal_in;
	
endmodule
