--------------------------------------------------------------------------------------------------------
--
-- pumpkin.vhd
-- 'a small general purpose, scalable, 16-bit, 16 instruction CPU core written in VHDL'
-- Initial release: 31 October 2020
--
--------------------------------------------------------------------------------------------------------
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
--------------------------------------------------------------------------------------------------------
--
--   15    14    13    12    11    10    9     8     7     6     5     4     3     2     1     0
--  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
--  |        Op-Code        |                        Program memory address                         |
--  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
--  
--  A   = Accumulator 
--  C   = Carry flag 
--  AC  = Carry flag with accumulator 
--  X   = 12-bit memory address from the instruction word 
--  M   = Program memory referenced by X 
--  CM  = Carry flag with program memory 
--  IO  = IO memory 
--  ( ) = Memory Subscript 
--  
--  +-------+-----------+------+----------------------------+
--  |Op-code|Instruction|Cycles|Description                 |
--  +-------+-----------+------+----------------------------+
--  | 0x0   | LOAD      | 2    | A = M(X)                   |
--  | 0x1   | STORE     | 2    | M(X) = A                   |
--  | 0x2   | ADD       | 2    | AC = M(X) + A              |
--  | 0x3   | SUB       | 2    | AC = M(X) - A              |
--  | 0x4   | OR        | 2    | A = M(X) OR A              |
--  | 0x5   | AND       | 2    | A = M(X) AND A             |
--  | 0x6   | XOR       | 2    | A = M(X) XOR A             |
--  | 0x7   | ROR       | 2    | AC = CM(X) >> 1            |
--  | 0x8   | SWAP      | 2    | A = M(X) [byte-swapped]    | 
--  | 0x9   | IN        | 2    | A = IO(M(X))               |
--  | 0xA   | OUT       | 2    | IO(M(X)) = A               |
--  | 0xB   | BR        | 1    | Branch always              |
--  | 0xC   | BNC       | 1    | Branch if carry flag is 0  |
--  | 0xD   | BNZ       | 1    | Branch if A is not 0       |
--  | 0xE   | CALL      | 1    | CALL subroutine            |
--  | 0xF   | RETURN    | 1    | RETURN from subroutine     |
--  +-------+-----------+------+----------------------------+
--
----------------------------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity pumpkin is
	generic(
		stack_depth     : integer := 4;
		program_size    : integer := 12);
	port (
		clock 			 : in std_logic;
		clock_enable	 : in std_logic;
		reset 			 : in std_logic;
		program_data_in : in std_logic_vector(15 downto 0);
		data_out        : out std_logic_vector(15 downto 0);
		program_address : out std_logic_vector(program_size-1 downto 0);
		program_wr      : out std_logic;
		io_data_in      : in std_logic_vector(15 downto 0);
		io_address      : out std_logic_vector(15 downto 0);
		io_rd           : out std_logic;
		io_wr           : out std_logic);
end entity;

architecture rtl of pumpkin is

	type state_type is (S0,S1,S2);
	signal state      : state_type;
	signal next_state : state_type;
	
	type callstack_type is array(0 to stack_depth-1) of std_logic_vector(program_size-1 downto 0);
	signal callstack : callstack_type;
	
	signal pc       : std_logic_vector(program_size-1 downto 0);
	signal ir       : std_logic_vector(3 downto 0);
	signal a        : std_logic_vector(15 downto 0);
	signal c        : std_logic;
	signal adder    : unsigned(16 downto 0);
	signal adder_a  : unsigned(16 downto 0);
	signal adder_b  : unsigned(16 downto 0);
	signal carry_in : std_logic;
	signal next_pc  : std_logic_vector(program_size-1 downto 0); 
	
	signal program_address_buffer : std_logic_vector(program_size-1 downto 0);
	
	constant I_LOAD   : std_logic_vector(3 downto 0) := "0000";
	constant I_STORE  : std_logic_vector(3 downto 0) := "0001";
	constant I_ADD    : std_logic_vector(3 downto 0) := "0010";
	constant I_SUB    : std_logic_vector(3 downto 0) := "0011";
	constant I_OR     : std_logic_vector(3 downto 0) := "0100";
	constant I_AND    : std_logic_vector(3 downto 0) := "0101";
	constant I_XOR    : std_logic_vector(3 downto 0) := "0110";
	constant I_ROR    : std_logic_vector(3 downto 0) := "0111";
	constant I_SWAP   : std_logic_vector(3 downto 0) := "1000";
	constant I_IN     : std_logic_vector(3 downto 0) := "1001";
	constant I_OUT    : std_logic_vector(3 downto 0) := "1010";
	constant I_BR     : std_logic_vector(3 downto 0) := "1011";
	constant I_BNC    : std_logic_vector(3 downto 0) := "1100";
	constant I_BNZ    : std_logic_vector(3 downto 0) := "1101";
	constant I_CALL   : std_logic_vector(3 downto 0) := "1110";
	constant I_RETURN : std_logic_vector(3 downto 0) := "1111";
	
begin

	program_address <= program_address_buffer;
	data_out <= a;
	io_address <= program_data_in;
	
	--
	-- RAM address source 
	--
	
	process(state,program_data_in,callstack(0),pc,a,c)
	begin
		case state is
			when S0 =>
				program_address_buffer <= (others=>'0');
			when S1 =>
				case program_data_in(15 downto 12) is
					when I_RETURN =>
						program_address_buffer <= callstack(0);
					when I_BNZ =>
						if a = X"0000" then		
							program_address_buffer <= pc; 
						else
							program_address_buffer <= program_data_in(program_size-1 downto 0);							
						end if;
					when I_BNC =>
						if c = '1' then
							program_address_buffer <= pc;
						else
							program_address_buffer <= program_data_in(program_size-1 downto 0);							
						end if;
					when others =>
						program_address_buffer <= program_data_in(program_size-1 downto 0);
				end case;
			when S2 =>
				program_address_buffer <= pc;
		end case;
	end process;
	
	--
	-- Call Stack
	--
	
	process(clock)
	begin
		if rising_edge(clock) then
			if clock_enable = '1' and state = S1 then
				case program_data_in(15 downto 12) is
					when I_RETURN =>
						for i in 1 to stack_depth-1 loop
							callstack(i-1) <= callstack(i);
						end loop;
					when I_CALL =>
						callstack(0) <= pc;
						for i in 1 to stack_depth-1 loop
							callstack(i) <= callstack(i-1);
						end loop;
					when others =>
						null;
				end case;
			end if;
		end if;
	end process;
	
	--
	-- State machine and Program Counter
	--
	
	next_pc <= std_logic_vector(unsigned(program_address_buffer) + 1);
	
	process(clock)
	begin
		if rising_edge(clock) then
			if reset = '1' then
				state <= S0;
			elsif clock_enable = '1' then
				case state is
					when S0 =>
						state <= S1;
						pc <= next_pc;
					when S1 =>
						case program_data_in(15 downto 12) is
							when I_BR|I_BNC|I_BNZ|I_CALL|I_RETURN =>
								state <= S1;
								pc <= next_pc;
							when others =>
								ir <= program_data_in(15 downto 12);
								state <= S2;
						end case;
					when S2 =>
						state <= S1;
						pc <= next_pc;
				end case;
			end if;
		end if;
	end process;
	
	--
	-- Memory control
	--
	
	program_wr <= '1' when clock_enable = '1' and state = S1 and program_data_in(15 downto 12) = I_STORE else '0';
	
	process(clock)
	begin
		if rising_edge(clock) then
			if clock_enable = '1' then
				-- IO Write (OUT)
				if state = S1 and program_data_in(15 downto 12) = I_OUT then
					io_wr <= '1';
				else
					io_wr <= '0';
				end if;
				-- IO Read (IN)
				if state = S1 and program_data_in(15 downto 12) = I_IN then
					io_rd <= '1';
				else
					io_rd <= '0';
				end if;
			end if;
		end if;
	end process;
	
	--
	-- ALU
	--
	
	process(clock)
	begin
		if rising_edge(clock) then
			if clock_enable = '1' and state = S2 then
				case ir is
					when I_LOAD =>
						a <= program_data_in;
					when I_ADD|I_SUB =>
						a <= std_logic_vector(adder(15 downto 0));
						c <= adder(16);
					when I_OR =>
						a <= a or program_data_in;
					when I_AND =>
						a <= a and program_data_in;
					when I_XOR =>
						a <= a xor program_data_in;
					when I_ROR =>
						a <= c & program_data_in(15 downto 1);
						c <= program_data_in(0);
					when I_SWAP =>
						a <= program_data_in(7 downto 0) & program_data_in(15 downto 8);
					when I_IN =>
						a <= io_data_in;
					when others => null;
				end case;
			end if;
		end if;
	end process;
	
	--
	-- Add / Subtract
	--
	
	adder_a <= unsigned('0' & a);
	adder_b <= unsigned('0' & not program_data_in) when ir = I_SUB else unsigned('0' & program_data_in);
	carry_in <= '1' when ir = I_SUB else '0';
	adder <= adder_a + adder_b + ((16 downto 1 => '0') & carry_in);
						
end rtl;

-- End of file

