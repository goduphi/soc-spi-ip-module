module chipselect_select
(
	input clk, reset, enable, cs_auto, cs_enable, cs_pull_low, cs_pull_high, select,
	output reg cs
);
	// Controls chipselect
	// Initialization steps
	// 1. Write 1 to bit 9 of the control register
	// 2. Enable the tx/rx/brd by setting bit 15
	always @ (posedge clk)
		if(reset || !enable)
			// By default, when the tx/rx/brd is not enabled, CS idle's high.
			cs <= 1'b1;
			// This state is only entered when CS is auto as defined by the state machine.
			// Pull CS Low by going into the CS Assert state.
		else
			if(cs_auto)
			begin
				if(cs_pull_low && select)
					cs <= 1'b0;
				// If CS is auto and we are in the TX_RX state and bcount is 0, pull CS High.
				// This ensures CS is pulled high as soon as we go back to the IDLE state.
				else if(cs_pull_high && select)
					cs <= 1'b1;
			end
			// If it is not CS auto, it has to be manual.
			// CS is controlled by the user.
			else if(select)
				cs <= cs_enable;
endmodule
