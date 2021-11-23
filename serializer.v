module serializer
(
	input clk, reset
);

	parameter IDLE    = 2'b00;
	parameter TX_RX   = 2'b01;
	parameter CS_AUTO = 2'b10;
	
endmodule
