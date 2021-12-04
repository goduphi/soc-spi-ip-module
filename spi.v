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
	output reg clk_out;
	output wire tx, baud_out, cs_0, cs_1, cs_2, cs_3;
	input rx;
	
	output wire [9:0] LEDR;
	
	// Register list
	parameter DATA_REG 		= 2'b00;
	parameter STATUS_REG 	= 2'b01;
	parameter CONTROL_REG 	= 2'b10;
	parameter BRD_REG			= 2'b11;
	
	// Internal registers
	reg [31:0] latch_data;
	reg [31:0] control;
	reg [31:0] brd;
	reg [31:0] clear_status_flag_request;
	
	// Only output the clock of the baud rate generator if bit 15 of the control register is set
	assign enable = control[15];
	
	// Read block
	always @ (*)
	begin
		if(read && chipselect)
			case(address)
				DATA_REG:
					readdata = tmp;
				STATUS_REG:
					// rp and wp are the read and write pointers
					// These are only here for the purposes of debugging
					readdata = { cs_auto, rp, wp, 2'b00, txfe, txff, txfo, rxfe, rxff, rxfo };
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
	wire [31:0] tx_fifo_data_out, rx_fifo_data_out;
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
		.ov_clear(clear_status_flag_request[3])
	);
	
	fifo #(.N(16), .M(32)) rx_fifo
	(
		.data_out(rx_fifo_data_out),
		.fe(rxfe),
		.ff(rxff),
		.fo(rxfo),
		.data_in(latch_data),
		.clk(clk),
		.reset(reset),
		.chipselect(chipselect || serializer_read_pulse),
		.read(one_pulse_read_output && (address == DATA_REG)),
		.write(serializer_read_pulse),
		.ov_clear(clear_status_flag_request[0]),
		.rp_debug_out(rp),
		.wp_debug_out(wp)
	);
	
	reg [31:0] tmp;
	always @ (posedge clk)
	begin
		if(chipselect && read && (address == DATA_REG))
			tmp <= rx_fifo_data_out;
	end
	
	/*
		There is a major problem with the chipselect. For some reason, after chipselect goes low,
		it takes a few clock cycles for the baud rate generator to be intialized.
	*/
	
	// Serializer
	parameter IDLE    	= 2'b00;
	parameter TX_RX   	= 2'b01;
	parameter CS_ASSERT 	= 2'b10;
	
	reg [1:0] serializer_state;
	
	// If in manual CS mode, the SPI Chipselect is going to be
	// controlled by CSy_Enable. For now, let's just assume that
	// we have only 1 chipselect.
	
	wire [3:0] cs_auto;
	wire [3:0] cs_enable;
	
	assign cs_auto = control[8:5];
	assign cs_enable = control[12:9];
	
	chipselect_select chipselect_0
	(
		.clk(clk),
		.reset(reset),
		.enable(enable),
		.cs_auto(cs_auto[0]),
		.cs_enable(cs_enable[0]),
		.cs_pull_low((serializer_state == CS_ASSERT)),
		.cs_pull_high((serializer_state == TX_RX) && (bcount == 0)),
		.select(control[14:13] == 2'b00),
		.cs(cs_0)
	);
	
	chipselect_select chipselect_1
	(
		.clk(clk),
		.reset(reset),
		.enable(enable),
		.cs_auto(cs_auto[1]),
		.cs_enable(cs_enable[1]),
		.cs_pull_low((serializer_state == CS_ASSERT)),
		.cs_pull_high((serializer_state == TX_RX) && (bcount == 0)),
		.select(control[14:13] == 2'b01),
		.cs(cs_1)
	);
	
	chipselect_select chipselect_2
	(
		.clk(clk),
		.reset(reset),
		.enable(enable),
		.cs_auto(cs_auto[2]),
		.cs_enable(cs_enable[2]),
		.cs_pull_low((serializer_state == CS_ASSERT)),
		.cs_pull_high((serializer_state == TX_RX) && (bcount == 0)),
		.select(control[14:13] == 2'b10),
		.cs(cs_2)
	);
	
	chipselect_select chipselect_3
	(
		.clk(clk),
		.reset(reset),
		.enable(enable),
		.cs_auto(cs_auto[3]),
		.cs_enable(cs_enable[3]),
		.cs_pull_low((serializer_state == CS_ASSERT)),
		.cs_pull_high((serializer_state == TX_RX) && (bcount == 0)),
		.select(control[14:13] == 2'b11),
		.cs(cs_3)
	);
	
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
	
	reg spo, sph;
	// Polarity, Phase selector
	always @ (*)
	begin
		case(control[14:13])
			2'b00: begin spo = control[16]; sph = control[17]; end
			2'b01: begin spo = control[18]; sph = control[19]; end
			2'b10: begin spo = control[20]; sph = control[21]; end
			2'b11: begin spo = control[22]; sph = control[23]; end
		endcase
	end
	
	assign baud_out = internal_baud_out ^ spo ^ sph;
	assign baud_out_idle_level = !(spo == 1'b0);
	
	wire baud_out_negative_edge;
	edge_detect baud_negative_edge
	(
		.clk(clk),
		.reset(reset),
		.signal_in(baud_out),
		.pulse_out(baud_out_negative_edge),
		
		// Phase xor'ed with Polarity if 0,0 and 1,1 produces zero.
		// That indicates we want to decrement the count on the negative edge.
		// Otherwise, decrement on the posedge.
		.positive_edge((spo ^ sph))
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
			// If CS Auto is selected, if in IDLE, go to the Assert state.
			// If in assert, go to the TX_RX state
			else if(!txfe && (cs_auto != 0))
			begin
				if(serializer_state == IDLE)
					serializer_state <= CS_ASSERT;
				else
					serializer_state <= TX_RX;
			end
			else if(!txfe && (cs_auto == 0))
				serializer_state <= TX_RX;
		end
	end
	
	assign load = (serializer_state == IDLE);
	// This decrement will always happen on the first negative edge. That causes a problem as the
	// count is decremented/incremented first and then data is transmitted. What I really want is for the first
	// bit to be sent and then decrement/increment the count.
	assign decrement = ((serializer_state == TX_RX) && baud_out_negative_edge);
	// The bcount should be compared against 0 because we want to go to the next
	// read value after the last bit from the data word is sent
	assign serializer_read_pulse = ((serializer_state == TX_RX) && bcount == 0);
	assign tx = internal_tx;
	
	reg internal_tx;
	// This block will not run until the baud rate generator is enabled
	always @ (baud_out)
	begin
		if(((baud_out && !(spo ^ sph)) || (!baud_out && (spo ^ sph))) && serializer_state == TX_RX && bcount > 0)
		begin
			internal_tx <= tx_fifo_data_out[bcount - 1'b1];
			latch_data[bcount - 1'b1] <= rx;
		end
		else
			internal_tx <= 1'b0;
	end

	// The baud_out always has to idle either high or low.
	// If we are in the idle state, idle baud_out to either high or low
	// Baud rate generator
	reg [31:0] count;
	reg [31:0] match;
	reg internal_baud_out;
	
	always @ (posedge clk)
	begin
		if(reset || !enable || (serializer_state == IDLE))
		begin
			clk_out <= 1'b0;
			// This should be passed in as a parameter
			internal_baud_out <= baud_out_idle_level;
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
				internal_baud_out <= !internal_baud_out;
			end
		end
	end
	
endmodule
