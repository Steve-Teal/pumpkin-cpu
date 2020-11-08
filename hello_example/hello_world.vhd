---------------------------------------------------------------------
--
-- Built with PASM version 1.3
-- File name: hello_world.vhd
-- 8-11-2020 13:06:24
-- 
---------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity hello_world is
port (
    clock        : in std_logic;
    clock_enable : in std_logic;
    address      : in std_logic_vector(6 downto 0);
    data_out     : out std_logic_vector(15 downto 0);
    data_in      : in std_logic_vector(15 downto 0);
    write_enable : in std_logic);
end entity;

architecture rtl of hello_world is

    type ram_type is array (0 to 127) of std_logic_vector(15 downto 0);
    signal ram : ram_type := (
			X"B017",X"7075",X"6D70",X"6B69",X"6E2D",X"6370",X"7520",X"6465",
			X"6D6F",X"0D0A",X"4865",X"6C6C",X"6F20",X"576F",X"726C",X"6421",
			X"0D0A",X"0000",X"001D",X"0000",X"0000",X"0000",X"0000",X"0045",
			X"A015",X"0046",X"3045",X"D01A",X"0045",X"E01F",X"B01E",X"1013",
			X"2013",X"1013",X"E029",X"D025",X"F000",X"E034",X"0013",X"2045",
			X"B021",X"1014",X"2047",X"7014",X"102D",X"B02E",X"C030",X"B032",
			X"1014",X"8014",X"5048",X"F000",X"1014",X"2014",X"4049",X"1014",
			X"004A",X"1016",X"0014",X"A015",X"7014",X"1014",X"0012",X"3045",
			X"D03F",X"0016",X"3045",X"D039",X"F000",X"0001",X"01F4",X"0000",
			X"00FF",X"0200",X"000A",X"0000",X"0000",X"0000",X"0000",X"0000",
			X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",
			X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",
			X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",
			X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",
			X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",X"0000",
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
