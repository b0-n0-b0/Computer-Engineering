library IEEE;
  use IEEE.std_logic_1164.all;

entity Counter_tb is  -- The testbench has no interface, so it is an empty entity (Be careful: the keyword "is" was missing in the code written in class).
end entity;

architecture bhv of Counter_tb is -- Testbench architecture declaration
  -----------------------------------------------------------------------------------
  -- Testbench constants
  -----------------------------------------------------------------------------------
  constant T_CLK    : time    := 10 ns;  -- Clock period
  constant T_RESET  : time    := 25 ns;  -- Period before the reset deassertion
  constant CNTR_BIT : natural := 8;

  -----------------------------------------------------------------------------------
  -- Testbench signals
  -----------------------------------------------------------------------------------
  signal clk_tb       : std_logic := '0';  -- clock signal, intialized to '0'
  signal a_rst_n      : std_logic := '0';  -- reset signal
  signal en_tb        : std_logic := '0';  -- enable signal to connect to the en port of the component
  signal increment_tb : std_logic_vector(CNTR_BIT - 1 downto 0) := (others => '0');  -- d signal to connect to the d port of the component
  signal cntr_out_tb  : std_logic_vector(CNTR_BIT - 1 downto 0);  -- q signal to connect to the q port of the component
  signal end_sim      : std_logic := '1';  -- signal to use to stop the simulation when there is nothing else to test

  -----------------------------------------------------------------------------------
  -- Component to test (DUT) declaration
  -----------------------------------------------------------------------------------
  component Counter is  -- be careful, it is only a component declaration. The component shall be instantiated after the keyword "begin" by linking the gates with the testbench signals for the test
    generic (
      N : natural := 8
    );
    port (
      clk       : in  std_logic;
      a_rst_n   : in  std_logic;
      increment : in  std_logic_vector(N - 1 downto 0);
      en        : in  std_logic;
      cntr_out  : out std_logic_vector(N - 1 downto 0)
    );
  end component;

begin

  clk_tb <= (not(clk_tb) and end_sim) after T_CLK / 2;  -- The clock toggles after T_CLK / 2 when end_sim is high. When end_sim is forced low, the clock stops toggling and the simulation ends.
  a_rst_n <= '1' after T_RESET;  -- Deasserting the reset after T_RESET nanosecods (remember: the reset is active low).

  Counter_DUT: Counter
    generic map (N => CNTR_BIT)
    port map (
      clk       => clk_tb,
      a_rst_n   => a_rst_n,
      increment => increment_tb,
      en        => en_tb,
      cntr_out  => cntr_out_tb
    );

  stimuli: process(clk_tb, a_rst_n)  -- process used to make the testbench signals change synchronously with the rising edge of the clock
    variable t : integer := 0;  -- variable used to count the clock cycle after the reset
  begin
    if (a_rst_n = '0') then
      increment_tb <= (others => '0');
      t := 0;
    elsif (rising_edge(clk_tb)) then

      case(t) is   -- specifying the input increment_tb and end_sim depending on the value of t ( and so on the number of the passed clock cycles).
        when 1  => increment_tb    <= "00000000"; -- increment_tb is forced to '0' when t = 1.
        when 2  => increment_tb    <= "00000001";
        when 10 => en_tb  <= '1';
                   increment_tb    <= "00000000";
        when 20 => increment_tb    <= "00000001";
        when 30 => increment_tb    <= "00000100";
        when 40 => increment_tb    <= "00001010";
        when 70 => end_sim <= '0';  -- This command stops the simulation
        when others => null;  -- Specifying that nothing happens in the other cases
      end case;

      t := t + 1;  -- the variable is updated exactly here (try to move this statement before the "case(t) is" one and watch the difference in the simulation)

    end if;
  end process;

end architecture;
