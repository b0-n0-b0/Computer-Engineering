library IEEE;
  use IEEE.std_logic_1164.all;

entity fifo_tb is
end entity;

architecture bhv of fifo_tb is -- Testbench architecture declaration
  -----------------------------------------------------------------------------------
  -- Testbench constants
  -----------------------------------------------------------------------------------
  constant T_CLK      : time    := 10 ns;  -- Clock period
  constant T_RESET    : time    := 25 ns;  -- Period before the reset deassertion
  constant DEPTH      : natural := 4;
  constant DATA_WIDTH : natural := 8;

  -----------------------------------------------------------------------------------
  -- Testbench signals
  -----------------------------------------------------------------------------------
  signal clk_tb       : std_logic := '0';  -- clock signal, intialized to '0'
  signal a_rst_n      : std_logic := '0';  -- reset signal
  signal data_in_ext  : std_logic_vector(DATA_WIDTH - 1 downto 0) := (others => '0');
  signal data_out_ext : std_logic_vector(DATA_WIDTH - 1 downto 0);
  signal end_sim      : std_logic := '1';

  -----------------------------------------------------------------------------------
  -- Component to test (DUT) declaration
  -----------------------------------------------------------------------------------
  component fifo is
    generic (
      DEPTH      : natural := 4;
      DATA_WIDTH : natural := 8
    );
    port (
      clk      : in  std_logic;
      a_rst_n  : in  std_logic;
      data_in  : in  std_logic_vector(DATA_WIDTH - 1 downto 0);
      data_out : out std_logic_vector(DATA_WIDTH - 1 downto 0)
    );
  end component;

begin

  clk_tb <= (not(clk_tb) and end_sim) after T_CLK / 2;  -- The clock toggles after T_CLK / 2 when end_sim is high. When end_sim is forced low, the clock stops toggling and the simulation ends.
  a_rst_n <= '1' after T_RESET;  -- Deasserting the reset after T_RESET nanosecods (remember: the reset is active low).

  DUT: fifo
    generic map (
      DEPTH       => DEPTH,
      DATA_WIDTH  => DATA_WIDTH
    )
    port map (
      clk      => clk_tb,
      a_rst_n  => a_rst_n,
      data_in  => data_in_ext,
      data_out => data_out_ext
    );

  STIMULI: process(clk_tb, a_rst_n)  -- process used to make the testbench signals change synchronously with the rising edge of the clock
    variable t : integer := 0;  -- variable used to count the clock cycle after the reset
  begin
    if(a_rst_n = '0') then
      data_in_ext <= (others => '0');
      t := 0;
    elsif(rising_edge(clk_tb)) then
      case(t) is
        when 1  => data_in_ext    <= "00000000";
        when 10 => data_in_ext    <= "10000000";
        when 20 => data_in_ext    <= "00000001";
        when 30 => data_in_ext    <= x"1";
        when 31 => data_in_ext    <= x"2";
        when 32 => data_in_ext    <= x"3";
        when 33 => data_in_ext    <= x"4";
        when 34 => data_in_ext    <= x"5";
        when 35 => data_in_ext    <= x"6";
        when 40 => end_sim <= '0';  -- This command stops the simulation when t = 10
        when others => null;        -- Specifying that nothing happens in the other cases
      end case;

      t := t + 1;  -- the variable is updated exactly here (try to move this statement before the "case(t) is" one and watch the difference in the simulation)
    end if;
  end process;

end architecture;
