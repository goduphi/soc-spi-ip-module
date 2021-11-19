// SPI IP Module
// Author: Sarker Nadir Afridi Azmi

module spi
(
	clk, reset, address, byteenable, chipselect, writedata, readdata, write, read,
	tx, rx, clk_out, baud_out, cs_0, cs_1, cs_2, cs_3, LEDR
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
	
	output wire [9:0] LEDR;
	
	// Register list
	parameter DATA_REG 		= 2'b00;
	parameter STATUS_REG 	= 2'b01;
	parameter CONTROL_REG 	= 2'b10;
	parameter BRD_REG			= 2'b11;
	
	// Internal registers
	reg [31:0] control;
	reg [31:0] brd;
	reg [31:0] clear_status_flag_request;
	reg [31:0] debug_reg;
	
	// Only output the clock of the baud rate generator if bit 15 of the control register is set
	assign enable = (control & 32'h00008000) ? 1'b1 : 1'b0;
	
	reg [M - 1:0] fifo_data_out, data_in;
	
	// Read block
	always @ (*)
	begin
		if(read && chipselect)
			case(address)
				DATA_REG:
					readdata = fifo_data_out;
				STATUS_REG:
					// rp and wp are the read and write pointers
					// These are only here for the purposes of debugging
					readdata = { rp[3:0], wp[3:0], 2'b00, txfe, txff, txfo, rxfe, rxff, rxfo };
				CONTROL_REG:
					readdata = control;
				BRD_REG:
					readdata = brd;
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
								// The status reg has 2 bits which are w1c
								// The idea is to set the clear flag for one clock
								// Then clear out the request
								STATUS_REG:
									clear_status_flag_request <= writedata;
                        CONTROL_REG:
									control <= writedata;
                        BRD_REG: 
                           brd <= writedata;
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
	wire one_pulse_write_output, one_pulse_read_output;
	
	edge_detect write_pos_edge_detect
	(
		.clk(clk),
		.reset(reset),
		.signal_in(write),
		.pulse_out(one_pulse_write_output)
	);
	
	edge_detect read_pos_edge_detect
	(
		.clk(clk),
		.reset(reset),
		.signal_in(read),
		.pulse_out(one_pulse_read_output)
	);
	
	// RX Fifo Overflow, Full, Empty; TX Fifo Overflow, Full, Empty
	wire txfe, txff, rxfe, rxff;
	reg txfo, rxfo;
	
	// TX Fifo Overflow Block
	always @ (posedge clk)
	begin
		if(reset)
			txfo <= 1'b0;
		// Bit 3 represents txfo w1c
		else if(clear_status_flag_request[3])
			txfo <= 1'b0;
		else
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
	
	// Fifo Block
	parameter M = 32;
	parameter N = 16;
	
	reg [M - 1:0] buffer [N - 1:0];
	reg [$clog2(N) - 1:0] rp, wp;
	reg [$clog2(N):0] pd;
	
	assign txfe = (pd == 1'b0) ? 1'b1 : 1'b0;
	assign txff = (pd == N) ? 1'b1 : 1'b0;
	
	assign LEDR[3:0] = wp;
	assign LEDR[7:4] = rp;
	
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
			if(chipselect && one_pulse_read_output && !txfe && (address == DATA_REG))
			begin
				fifo_data_out <= buffer[rp];
				rp <= rp + 1'b1;
				pd <= pd - 1'b1;
			end
			else if(chipselect && one_pulse_write_output && !txff && (address == DATA_REG))
			begin
				buffer[wp] <= writedata;
				wp <= wp + 1'b1;
				pd <= pd + 1'b1;
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
