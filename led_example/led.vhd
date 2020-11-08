---------------------------------------------------------------------
--
-- Built with PASM version 1.3
-- File name: led.vhd
-- 8-11-2020 15:11:14
-- 
---------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity led is
port (
    clock        : in std_logic;
    clock_enable : in std_logic;
    address      : in std_logic_vector(4 downto 0);
    data_out     : out std_logic_vector(15 downto 0);
    data_in      : in std_logic_vector(15 downto 0);
    write_enable : in std_logic);
end entity;

architecture rtl of led is

    type ram_type is array (0 to 31) of std_logic_vector(15 downto 0);
    signal ram : ram_type := (
			X"B003",X"0000",X"0000",X"0010",X"A001",X"0011",X"1002",X"0012",
			X"3010",X"D008",X"0002",X"3010",X"D006",X"9001",X"6010",X"B004",
			X"0001",X"001E",X"FFFF",X"0000",X"0000",X"0000",X"0000",X"0000",
			X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",X"0000");
begin

    process(clock)
    begin
        if rising_edge(clock) then
            if clock_enable = '1' then
                if write_enable = '1' then
                    ram(to_integer(unsigned(address))) <= data_in;
                else
                    data_out <= ram(to_integer(unsigned(address)));
                end if;
            end if;
        end if;
    end process;

end rtl;

--- End of file ---
