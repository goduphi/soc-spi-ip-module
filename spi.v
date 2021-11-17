// SPI IP Module
// Author: Sarker Nadir Afridi Azmi

module spi
(
	clk, reset, address, byteenable, chipselect, writedata, readdata, write, read,
	tx, rx, clk_out, baud_out, cs_0, cs_1, cs_2, cs_3
);
	
	// Clock, reset
	input   				clk, reset;

	// Avalon Memory Mapped interface (4 word aperature)
	input             read, write, chipselect;
	input [1:0]       address;
	input [3:0]       byteenable;
	input [31:0]      writedata;
	output reg [31:0] readdata;
	
	// SPI Interface
	output reg tx, clk_out, baud_out, cs_0, cs_1, cs_2, cs_3;
	input rx;
	
	// Register list
	parameter DATA_REG 		= 2'b00;
	parameter STATUS_REG 	= 2'b01;
	parameter CONTROL_REG 	= 2'b10;
	parameter BRD_REG			= 2'b11;
	
	// Internal registers
	// RX Fifo Overflow, Full, Empty; TX Fifo Overflow, Full, Empty
	reg txfe, txff, txfo, rxfe, rxff, rxfo;
	reg [31:0] control;
	reg [31:0] brd;
	reg [31:0] clear_status_flag_request;
	reg [31:0] debug_reg;
	
	// Only output the clock of the baud rate generator if bit 15 of the control register is set
	assign enable = (control & 32'h00008000) ? 1'b1 : 1'b0;
	
	// Read block
	always @ (*)
	begin
		if(read && chipselect)
			case(address)
				STATUS_REG:
					readdata = { 26'b0, txfe, txff, txfo, rxfe, rxff, rxfo };
				CONTROL_REG:
					readdata = control;
				BRD_REG:
					readdata = debug_reg;
			endcase
		else
			readdata = 32'b0;
	end
	
	// Write block
	always @ (posedge clk)
	begin
		if(reset)
		begin
			control <= 32'b0;
			brd <= 32'b0;
		end
		else
		begin
            begin
                if (write && chipselect)
                begin
                    case (address)
                        CONTROL_REG:
									control <= writedata;
                        BRD_REG: 
                           brd <= writedata;
								// The status reg has 2 bits which are w1c
								// The idea is to set the clear flag for one clock
								// Then clear out the request
								STATUS_REG:
									clear_status_flag_request <= writedata;
                    endcase
                end
					 else
						clear_status_flag_request <= 32'b0;
            end
        end
	end
	
	// TX FIFO Block
	/*
		Each time the write pulse is asserted, we only want to write into the
		FIFO only once.
		
		If write was 0 and write is one (There's the concept of last data), it
		is a positive edge.
		
		We are going to need the 1 pulse write module given below!
		This generates a positve edge.
	*/
	
	// One pulse generation Block
	wire one_pulse_write_output;
	
	edge_detect write_pos_edge_detect(
												.clk(clk),
												.reset(reset),
												.signal_in(write),
												.pulse_out(one_pulse_write_output)
												);
	
	always @ (posedge one_pulse_write_output)
	begin
		if(chipselect && (address == DATA_REG))
			debug_reg <= debug_reg + 32'b1;
	end
	
	// TXFO, RXFO Debug
	always @ (posedge clk)
	begin
		txff <= control[24];
	end
	
	// TXFO block
	// This connects to the TX FIFO
	always @ (posedge clk)
	begin
		if(reset)
			txfo <= 1'b0;
		// Bit 3 represents txfo w1c
		else if(clear_status_flag_request[3])
			txfo <= 1'b0;
		begin
			if(write && chipselect)
			begin
				// If the FIFO is full, and we are writing to the FIFO (Data Reg)
				// and if the overflow is not set, write a 1 to it.
				if(txff && !txfo && address == DATA_REG)
					txfo <= 1'b1;
				// If none of the above conditions are true, don't change
				// the value of the overflow flag.
			end
		end
	end
	
	// Baud rate generator
	reg [31:0] count;
	reg [31:0] match;
	
	always @ (posedge clk)
	begin
		if(reset || !enable)
		begin
			clk_out <= 1'b0;
			baud_out <= 1'b0;
			count <= 32'b0;
			match <= brd;
		end
		else
		begin
			// Toggle output
			clk_out <= !clk_out;
			// Increment by 1.0
			count <= count + 32'b10000000;
			if(count[31:7] == match[31:7])
			begin
				match <= match + brd;
				baud_out <= !baud_out;
			end
		end
	end
	
endmodule
