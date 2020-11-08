---------------------------------------------------------------------
--
-- Built with PASM version 1.3
-- File name: led.vhd
-- 8-11-2020 10:47:40
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
			X"0000",X"0000",X"000F",X"A000",X"0010",X"1001",X"0011",X"300F",
			X"D007",X"0001",X"300F",X"D005",X"9000",X"600F",X"B003",X"0001",
			X"001E",X"FFFF",X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",
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
