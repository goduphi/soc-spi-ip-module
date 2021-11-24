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
	output reg tx, clk_out, baud_out, cs_0, cs_2, cs_3;
	output wire cs_1;
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
	
	// Only output the clock of the baud rate generator if bit 15 of the control register is set
	assign enable = (control & 32'h00008000) ? 1'b1 : 1'b0;
	
	// Read block
	always @ (*)
	begin
		if(read && chipselect)
			case(address)
				DATA_REG:
					readdata = tx_fifo_data_out;
				STATUS_REG:
					// rp and wp are the read and write pointers
					// These are only here for the purposes of debugging
					readdata = { serializer_state, rp, wp, 2'b00, txfe, txff, txfo, rxfe, rxff, rxfo };
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
		.pulse_out(one_pulse_write_output),
		.positive_edge(1'b1)
	);
	
	edge_detect read_pos_edge_detect
	(
		.clk(clk),
		.reset(reset),
		.signal_in(read),
		.pulse_out(one_pulse_read_output),
		.positive_edge(1'b1)
	);
	
	// RX Fifo Overflow, Full, Empty; TX Fifo Overflow, Full, Empty
	wire txfe, txff, txfo, rxfe, rxff, rxfo;
	wire [31:0] tx_fifo_data_out;
	wire [3:0] rp, wp;
	
	// Debug outputs
	assign LEDR[3:0] = wp;
	assign LEDR[7:4] = rp;

	fifo #(.N(16), .M(32)) tx_fifo
	(
		.data_out(tx_fifo_data_out),
		.fe(txfe),
		.ff(txff),
		.fo(txfo),
		.data_in(writedata),
		.clk(clk),
		.reset(reset),
		.chipselect(chipselect || serializer_read_pulse),
		.read(serializer_read_pulse),
		.write(one_pulse_write_output && (address == DATA_REG)),
		.ov_clear(clear_status_flag_request[3]),
		.rp_debug_out(rp),
		.wp_debug_out(wp)
	);
	
	// Serializer
	parameter IDLE    	= 2'b00;
	parameter TX_RX   	= 2'b01;
	parameter CS_ASSERT 	= 2'b10;
	
	reg [1:0] serializer_state;
	
	// Debug output
	assign LEDR[9:8] = serializer_state;
	
	// If in manual CS mode, the SPI Chipselect is going to be
	// controlled by CSy_Enable. For now, let's just assume that
	// we have only 1 chipselect.
	assign cs_auto = control[5];
	
	// Controls chipselect
	// Debug
	always @ (posedge clk)
		if(reset)
			cs_0 <= 1'b0;
		else
			cs_0 <= control[9];
	
	/*
	// Counter block
	// Debug
	wire load, decrement;
	wire [4:0] load_value;
	reg  [5:0] bcount;
	
	always @ (posedge clk)
		if(reset)
			bcount <= 0;
		else
			if(load)
				bcount <= load_value + 1'b1;
			else if(decrement)
				bcount <= bcount - 1'b1;
	*/
	
	wire [5:0] bcount;
	
	counter serializer_counter
	(
		.clk(clk),
		.reset(reset),
		.load(load),
		.decrement(decrement),
		.load_value(control[4:0]),
		.bcount(bcount)
	);
	
	wire baud_out_negative_edge;
	
	edge_detect baud_negative_edge
	(
		.clk(clk),
		.reset(reset),
		.signal_in(baud_out),
		.pulse_out(baud_out_negative_edge),
		.positive_edge(1'b0)
	);
	
	// Controls the serializer state
	always @ (posedge clk)
	begin
		if(reset || !enable)
		begin
			serializer_state <= IDLE;
		end
		else
		begin
			// The empty flag causes the serializer to begin transmission
			if(txfe || bcount == 0)
				serializer_state <= IDLE;
			else if(!txfe && !cs_auto)
				serializer_state <= TX_RX;
		end
	end
	
	assign load = (serializer_state == IDLE);
	assign decrement = (serializer_state == TX_RX && baud_out_negative_edge);
	// The bcount should be compared against 0 because we want to go to the next
	// read value after the last bit from the data word is sent
	assign serializer_read_pulse = (serializer_state == TX_RX && bcount == 0);
	
	// This block will not run until the baud rate generator is enabled
	always @ (posedge baud_out)
	begin
		if(serializer_state == TX_RX && bcount > 0)
			tx <= tx_fifo_data_out[bcount - 1'b1];
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
