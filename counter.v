module counter
(
	input clk, reset, load, decrement,
	input [4:0] load_value,
	output reg  [5:0] bcount
);
	always @ (posedge clk)
		if(reset)
			bcount <= 0;
		else
			if(load)
				bcount <= load_value + 1'b1;
			else if(decrement)
				bcount <= bcount - 1'b1;

endmodule
