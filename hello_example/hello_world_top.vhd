---------------------------------------------------------------------------------------------------
--
-- hello_world_top.vhd
--
-- Top level Hello World! example for the pumpkin-cpu
--
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

entity hello_world_top is
    port (
        clock        : in std_logic;
        n_reset      : in std_logic;
        tx           : out std_logic);
end entity;

architecture rtl of hello_world_top is

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
	
	component hello_world is
		port (
			clock        : in std_logic;
			clock_enable : in std_logic;
			address      : in std_logic_vector(6 downto 0);
			data_out     : out std_logic_vector(15 downto 0);
			data_in      : in std_logic_vector(15 downto 0);
			write_enable : in std_logic);
	end component;

    signal reset       : std_logic;
    signal cpu_data    : std_logic_vector(15 downto 0);
    signal ram_data    : std_logic_vector(15 downto 0);
    signal io_data     : std_logic_vector(15 downto 0);
    signal ram_address : std_logic_vector(6 downto 0);
    signal io_address  : std_logic_vector(15 downto 0);
    signal ram_wr      : std_logic;
    signal io_rd       : std_logic;
    signal io_wr       : std_logic;
    signal reset_done  : std_logic;
    signal reset_counter : integer range 0 to 15;

begin

    reset <= not reset_done;

--
-- CPU instantiation 
--
u1: pumpkin generic map (
            stack_depth => 2,
            program_size => 7)
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
			
u2: hello_world port map (
		clock => clock,
		clock_enable => '1',
		address => ram_address,
		data_out => ram_data,
		data_in  => cpu_data,
		write_enable => ram_wr);
		
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
	-- IO Write (output)
	--
	process(clock)
	begin
		if rising_edge(clock) then
			if io_wr = '1' and io_address = X"0000" then
				tx <= cpu_data(0);
			end if;
		end if;
	end process;
	
end rtl;

--- End of file ---
