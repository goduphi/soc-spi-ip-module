module serializer
(
	input clk, reset,
	input [31:0] control
);

	parameter IDLE    = 2'b00;
	parameter TX_RX   = 2'b01;
	parameter CS_AUTO = 2'b10;
	
	reg [1:0] serializer_state;
	
endmodule
