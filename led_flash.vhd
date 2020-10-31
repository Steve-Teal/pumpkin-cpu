---------------------------------------------------------------------------------------------------
--
-- led_flash.vhd
--
-- Top level LED flash example for the pumpkin-cpu
--
-- The LED with flash approximately once per second with a 12MHz clock. For faster or slower
-- clock speeds the initial value of the outer loop counter can be adjusted.
--
---------------------------------------------------------------------------------------------------
--
-- This file is part of the pumpkin-cpu Project
-- Copyright (C) 2020 Steve Teal
-- 
-- This source file may be used and distributed without restriction provided that this copyright
-- statement is not removed from the file and that any derivative work contains the original
-- copyright notice and the associated disclaimer.
-- 
-- This source file is free software; you can redistribute it and/or modify it under the terms
-- of the GNU Lesser General Public License as published by the Free Software Foundation,
-- either version 3 of the License, or (at your option) any later version.
-- 
-- This source is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
-- without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
-- See the GNU Lesser General Public License for more details.
-- 
-- You should have received a copy of the GNU Lesser General Public License along with this
-- source; if not, download it from http://www.gnu.org/licenses/lgpl-3.0.en.html
--
---------------------------------------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity led_flash is
    port (
        clock        : in std_logic;
        n_reset      : in std_logic;
        led          : out std_logic);
end entity;

architecture rtl of led_flash is

	component pumpkin is	
		generic(
			stack_depth  : integer := 6;
			program_size : integer := 12);
		port (
			clock 			: in std_logic;
			clock_enable	: in std_logic;
			reset 			: in std_logic;
			program_data_in	: in std_logic_vector(15 downto 0);
			data_out        : out std_logic_vector(15 downto 0);
			program_address : out std_logic_vector(program_size-1 downto 0);
			program_wr      : out std_logic;
			io_data_in      : in std_logic_vector(15 downto 0);
			io_address      : out std_logic_vector(15 downto 0);
			io_rd           : out std_logic;
			io_wr           : out std_logic);  
	end component;

    type ram_type is array (0 to 31) of std_logic_vector(15 downto 0);
    signal ram : ram_type := (
			X"000D", -- 0x00: 0x000D    ; LOAD  [0x0D]  'Load A with 1'
            X"A00E", -- 0x01: 0xA00E    ; OUT   [0x0E]  'Write A to LED port, IO addresss 0'
            X"000F", -- 0x02: 0x000F    ; LOAD  [0x0F]  'Load A with 30, number of outer delay loop iterations'
            X"1011", -- 0x03: 0x1011    ; STORE [0x11]  'Store A in the outer counter loop variable'
            X"0010", -- 0x04: 0x0010    ; LOAD  [0x10]  'Load A with 65535'
            X"300D", -- 0x05: 0x300D    ; SUB   [0x0D]  'Subtract 1 from A'
            X"D005", -- 0x06: 0xD005    ; BNZ   [0x05]  'Branch to 0x05 if A is not zero'
            X"0011", -- 0x07: 0x0011    ; LOAD  [0x11]  'Load A from the outer counter variable'
			X"300D", -- 0x08: 0x300D    ; SUB   [0x0D]  'Subtract 1 from A'
            X"D003", -- 0x09: 0xD003    ; BNZ   [0x03]  'Branch to 0x03 if A is not zero'
            X"900E", -- 0x0A: 0x900E    ; IN    [0x0E]  'Load A (bit 0) with current status of LED'
            X"600D", -- 0x0B: 0x600D    ; XOR   [0x0D]  'XOR A with 1, inverting bit 0'
            X"B001", -- 0x0C: 0xB001    ; BR    [0x01]  'Branch to 0x01'
            X"0001", -- 0x0D: 0x0001    ; Constant 1 
            X"0000", -- 0x0E: 0x0000    ; Constant 0
            X"001E", -- 0x0F: 0x001E    ; Constant 30
			X"FFFF", -- 0x10: 0xFFFF    ; Constant 65535
            X"0000", -- 0x11: 0x0000    ; Outer loop counter variable
            -- Unused memory
            X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",
			X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",X"0000");

    signal reset       : std_logic;
    signal cpu_data    : std_logic_vector(15 downto 0);
    signal ram_data    : std_logic_vector(15 downto 0);
    signal io_data     : std_logic_vector(15 downto 0);
    signal ram_address : std_logic_vector(4 downto 0);
    signal io_address  : std_logic_vector(15 downto 0);
    signal ram_wr      : std_logic;
    signal io_rd       : std_logic;
    signal io_wr       : std_logic;
	signal led_reg     : std_logic;
	signal reset_done  : std_logic;
	
	signal reset_counter : integer range 0 to 15;

begin

    reset <= not reset_done;
	led <= led_reg;

--
-- CPU instantiation 
--
u1: pumpkin generic map (
            stack_depth => 2,
            program_size => 5)
        port map(
            clock => clock,
            clock_enable => '1',
            reset => reset,
            program_data_in => ram_data,
            data_out => cpu_data,
            program_address => ram_address,
            program_wr => ram_wr,
            io_data_in => io_data,
            io_address => io_address,
            io_rd => io_rd,
            io_wr => io_wr);
				
	--
	-- Reset timer
	--
	process(clock)
	begin
		if rising_edge(clock) then
			if n_reset = '0' then
				reset_counter <= 0;
				reset_done <= '0';
			elsif reset_counter = 15 then
				reset_done <= '1';
			else
				reset_counter <= reset_counter + 1;
			end if;
		end if;
	end process;

	--
	-- Program RAM
	--
    process(clock)
    begin
        if rising_edge(clock) then
            if ram_wr = '1' then
                ram(to_integer(unsigned(ram_address))) <= cpu_data;
            else
                ram_data <= ram(to_integer(unsigned(ram_address)));
            end if;
        end if;
    end process;
	
	--
	-- IO Read (input)
	--
	io_data <= (15 downto 1 => '0') & led_reg when io_address = X"0000" and io_rd = '1' else (others=>'0');
	
	--
	-- IO Write (output)
	--
	process(clock)
	begin
		if rising_edge(clock) then
			if io_wr = '1' and io_address = X"0000" then
				led_reg <= cpu_data(0);
			end if;
		end if;
	end process;
	
end rtl;

--- End of file ---
