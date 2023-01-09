library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

entity full_adder_tb is
end full_adder_tb;

architecture beh of full_adder_tb is

  constant CLK_PERIOD : time := 100 ns;

  component full_adder
    port (
      a    : in  std_logic;
      b    : in  std_logic;
      cin  : in  std_logic;
      s    : out std_logic;
      cout : out std_logic
    );
  end component;

  signal clk      : std_logic := '0';
  signal a_ext    : std_logic := '0';
  signal b_ext    : std_logic := '0';
  signal cin_ext  : std_logic := '0';
  signal s_ext    : std_logic;
  signal cout_ext : std_logic;
  signal testing  : boolean := true;

begin

  clk <= not clk after CLK_PERIOD/2 when testing else '0';

  DUT: full_adder
    port map(
      a    => a_ext,
      b    => b_ext,
      cin  => cin_ext,
      s    => s_ext,
      cout => cout_ext
    );

  STIMULUS: process (clk)
    variable t : integer := 0;
  begin
    if rising_edge(clk) then
      case t is
        when 0 =>
          a_ext   <= '0';
          b_ext   <= '0';
          cin_ext <= '0';
        
        when 1 =>
          a_ext   <= '1';
          b_ext   <= '0';
          cin_ext <= '0';
        
        when 2 =>
          a_ext   <= '0';
          b_ext   <= '1';
          cin_ext <= '1';
        
        when 3 =>
          a_ext   <= '1';
          b_ext   <= '1';
          cin_ext <= '1';
        
        when 5 =>
          a_ext   <= '1';
          b_ext   <= '1';
          cin_ext <= '0';
        
        when 10 =>
          a_ext   <= '0';
          b_ext   <= '1';
          cin_ext <= '0';

        when 15 =>
          testing <= false;
      end case;

      t := t + 1;
    end if;
  end process;

end architecture;
