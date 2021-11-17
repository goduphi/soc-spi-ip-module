module edge_detect
(
	input clk, reset, signal_in,
	output reg pulse_out
);
	// One pulse generation Block
	reg old_write;
	always @ (posedge clk)
	begin
		if(reset)
		begin
			old_write <= 1'b0;
			pulse_out <= 1'b0;
		end
		else
		begin
			// Store the 1 clock delayed write value into a register
			// We only want this signal to be assert for 1 clock
			// So, auto clear it
			old_write <= signal_in;
			if(!old_write && signal_in)
				pulse_out <= 1'b1;
			else
				pulse_out <= 1'b0;
		end
	end
endmodule
