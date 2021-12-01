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
	output reg tx, clk_out, baud_out;
	output wire cs_0, cs_1, cs_2, cs_3;
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
	
	assign cs_0_enable = control[9];
	assign cs_1_enable = control[10];
	assign cs_2_enable = control[11];
	assign cs_3_enable = control[12];
	
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
		.cs_enable(cs_0_enable),
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
		.cs_enable(cs_1_enable),
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
		.cs_enable(cs_2_enable),
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
		.cs_enable(cs_3_enable),
		.cs_pull_low((serializer_state == CS_ASSERT)),
		.cs_pull_high((serializer_state == TX_RX) && (bcount == 0)),
		.select(control[14:13] == 2'b11),
		.cs(cs_3)
	);

	/*
	// Controls chipselect
	// Initialization steps
	// 1. Write 1 to bit 9 of the control register
	// 2. Enable the tx/rx/brd by setting bit 15
	always @ (posedge clk)
		if(reset || !enable)
		begin
			// By default, when the tx/rx/brd is not enabled, CS idle's high.
			cs_0 <= 1'b1;
			cs_1 <= 1'b1;
			cs_2 <= 1'b1;
			cs_3 <= 1'b1;
		end
			// This state is only entered when CS is auto as defined by the state machine.
			// Pull CS Low by going into the CS Assert state.
		else
			// If it is not CS auto, it has to be manual
			if(cs_auto != 4'b0)
			begin
				if(serializer_state == CS_ASSERT)
					case({ cs_select, cs_auto })
						6'b000001: cs_0 <= 1'b0;
						6'b010010: cs_1 <= 1'b0;
						6'b100100: cs_2 <= 1'b0;
						6'b111000: cs_3 <= 1'b0;
					endcase
					// cs <= 1'b0;
				// If CS is auto and we are in the TX_RX state and bcount is 0, pull CS High.
				// This ensures CS is pulled high as soon as we go back to the IDLE state.
				else if(serializer_state == TX_RX && bcount == 0)
					case({ cs_select, cs_auto })
						6'b000001: cs_0 <= 1'b1;
						6'b010010: cs_1 <= 1'b1;
						6'b100100: cs_2 <= 1'b1;
						6'b111000: cs_3 <= 1'b1;
					endcase
			end
			// Otherwise, CS is controlled by the user.
			else
				begin
					cs_0 <= control[9];
					cs_1 <= control[10];
					cs_2 <= control[11];
					cs_3 <= control[12];
				end
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
	assign decrement = ((serializer_state == TX_RX) && baud_out_negative_edge);
	// The bcount should be compared against 0 because we want to go to the next
	// read value after the last bit from the data word is sent
	assign serializer_read_pulse = ((serializer_state == TX_RX) && bcount == 0);
	
	// This block will not run until the baud rate generator is enabled
	always @ (posedge baud_out)
	begin
		if(serializer_state == TX_RX && bcount > 0)
			tx <= tx_fifo_data_out[bcount - 1'b1];
		else
			tx <= 1'b0;
	end

	// The baud_out always has to idle either high or low.
	// If we are in the idle state, idle baud_out to either high or low
	// Baud rate generator
	reg [31:0] count;
	reg [31:0] match;
	
	always @ (posedge clk)
	begin
		if(reset || !enable || (serializer_state == IDLE))
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
