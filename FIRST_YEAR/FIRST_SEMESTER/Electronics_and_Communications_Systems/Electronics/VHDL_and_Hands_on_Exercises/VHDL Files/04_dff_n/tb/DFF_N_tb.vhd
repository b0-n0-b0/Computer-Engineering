library IEEE;
  use IEEE.std_logic_1164.all;

-- The testbench has no interface, so it is an empty entity.
entity DFF_N_tb is
end entity;

architecture bhv of DFF_N_tb is -- Testbench architecture declaration
  -----------------------------------------------------------------------------------
  -- Testbench constants
  -----------------------------------------------------------------------------------
  constant T_CLK   : time    := 10 ns;  -- Clock period
  constant T_RESET : time    := 25 ns;  -- Period before the reset deassertion
  constant DFF_BIT : natural := 8;

  -----------------------------------------------------------------------------------
  -- Testbench signals
  -----------------------------------------------------------------------------------
  signal clk_tb   : std_logic := '0';  -- clock signal, intialized to '0'
  signal arstn_tb : std_logic := '0';  -- asynchronous reset low signal
  signal en_tb    : std_logic := '0';  -- enable signal to connect to the en port of the component
  signal d_tb     : std_logic_vector(DFF_BIT - 1 downto 0) := (others => '0');  -- d signal to connect to the d port of the component
  signal q_tb     : std_logic_vector(DFF_BIT - 1 downto 0);  -- q signal to connect to the q port of the component
  signal end_sim  : std_logic := '1';  -- signal to use to stop the simulation when there is nothing else to test

  -----------------------------------------------------------------------------------
  -- Component to test (DUT) declaration
  -----------------------------------------------------------------------------------
  component DFF_N is  -- be careful, it is only a component declaration. The component shall be instantiated after the keyword "begin" by linking the gates with the testbench signals for the test
    generic (
      N : natural := 8
    );
    port (
      clk      : in std_logic;
      arstn_tb : in std_logic;
      en       : in std_logic;
      d        : in std_logic_vector(N - 1 downto 0);
      q        : out std_logic_vector(N - 1 downto 0)
    );
  end component;

begin

  clk_tb   <= not(clk_tb) and end_sim after T_CLK / 2;  -- The clock toggles after T_CLK / 2 when end_sim is high. When end_sim is forced low, the clock stops toggling and the simulation ends.
  arstn_tb <= '1' after T_RESET;  -- Deasserting the reset after T_RESET nanosecods (remember: the reset is active low).

  DUT : DFF_N
    generic map (N => DFF_BIT)
    port map (
      clk     => clk_tb,
      arstn_tb => arstn_tb,
      en      => en_tb,
      d       => d_tb,
      q       => q_tb
    );

  STIMULI : process(clk_tb, arstn_tb)  -- process used to make the testbench signals change synchronously with the rising edge of the clock
    variable t : integer := 0;  -- variable used to count the clock cycle after the reset
  begin
    if arstn_tb = '0' then
      d_tb <= (others => '0');
      t := 0;
    elsif rising_edge(clk_tb) then
      case(t) is  -- specifying the input d_tb and end_sim depending on the value of t ( and so on the number of the passed clock cycles).
        when 1  => d_tb    <= "00000000";  -- d_tb is forced to '0' when t = 1.
        when 2  => d_tb    <= "10101010";
        when 3  => d_tb    <= "00000000";
        when 4  => en_tb   <= '1';
        when 5  => d_tb    <= std_logic_vector(to_unsigned(68, DFF_BIT));
        when 6  => d_tb    <= "00000000";
        when 10 => end_sim <= '0';  -- This command stops the simulation when t = 10
        when others => null;        -- Specifying that nothing happens in the other cases
      end case;

      t := t + 1;  -- the variable is updated exactly here (try to move this statement before the "case(t) is" one and watch the difference in the simulation)
    end if;
  end process;

end architecture;
