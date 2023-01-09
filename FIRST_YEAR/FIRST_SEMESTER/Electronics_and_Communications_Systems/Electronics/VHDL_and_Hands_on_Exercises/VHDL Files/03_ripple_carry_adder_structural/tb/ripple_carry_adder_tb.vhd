library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

entity ripple_carry_adder_tb is
end entity;

architecture beh of ripple_carry_adder_tb is

  constant clk_period : time     := 100 ns;
  constant N          : positive := 8;

  component ripple_carry_adder is
    generic (
      Nbit : positive := 8
    );
    port (
      a    : in  std_logic_vector(Nbit-1 downto 0);
      b    : in  std_logic_vector(Nbit-1 downto 0);
      cin  : in  std_logic;
      s    : out std_logic_vector(Nbit-1 downto 0);
      cout : out std_logic
    );
  end component;

  signal clk       : std_logic := '0';
  signal resetn    : std_logic := '0';
  signal a_ext     : std_logic_vector(N-1 downto 0) := (others => '0');
  signal b_ext     : std_logic_vector(N-1 downto 0) := (others => '0');
  signal c_in_ext  : std_logic := '0';
  signal s_ext     : std_logic_vector(N-1 downto 0);
  signal c_out_ext : std_logic;
  signal testing   : boolean := true;

begin

  clk    <= not clk after clk_period/2 when testing else '0';
  resetn <= '1' after 5 * clk_period;

  DUT: ripple_carry_adder
    generic map (
      Nbit => N
    )
    port map (
      a    => a_ext,
      b    => b_ext,
      cin  => c_in_ext,
      s    => s_ext,
      cout => c_out_ext
    );

  STIMULI : process (clk, resetn)
    variable t : integer := 0;
  begin
    if resetn = '0' then
      a_ext    <= (others => '0');
      b_ext    <= (others => '0');
      c_in_ext <= '0';
      t := 0;
    elsif rising_edge(clk) then
      case t is
        when 2  =>
          a_ext    <= "00000110";
          b_ext    <= "00100110";
          c_in_ext <= '0';

        when 5  =>
          a_ext    <= x"76";
          b_ext    <= x"19";
          c_in_ext <= '1';

        when 10 =>
          a_ext    <= (others => '0');
          b_ext    <= (others => '0');
          c_in_ext <= '0';

        when 15 =>
          a_ext    <= "11111111";
          b_ext    <= std_logic_vector(to_unsigned(58, N));
          c_in_ext <= '0';

        when 20 =>
          testing <= false;
      end case;

      t := t + 1;
    end if;
  end process;

end architecture;
