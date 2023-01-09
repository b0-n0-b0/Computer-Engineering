library IEEE;
  use IEEE.std_logic_1164.all;

entity DDFS is
  port(
    clk   : in std_logic;  -- clock of the system
    reset : in std_logic;  -- Asynchronous reset - active high

    fw : in std_logic_vector(11 downto 0);  -- input frequency word
    yq : out std_logic_vector(5 downto 0)   -- output waveform
  );
end entity;

architecture struct of DDFS is
-------------------------------------------------------------------------------------
-- Internal signals
-------------------------------------------------------------------------------------

  -- Output of of the phase accumulator counter
  signal phase_out : std_logic_vector(11 downto 0);

  -- Output of the LUT table
  signal lut_output : std_logic_vector(5 downto 0);

  -- Output register for the output synchronization
  signal output_reg : std_logic_vector(5 downto 0);


-------------------------------------------------------------------------------------
-- Internal Component
-------------------------------------------------------------------------------------
  component Counter is
    generic ( N : natural := 8 );
    port (
      clk     : in  std_logic;
      a_rst_h : in  std_logic;

      en        : in  std_logic;
      increment : in  std_logic_vector(N - 1 downto 0);
      cntr_out  : out std_logic_vector(N - 1 downto 0)
    );
  end component;

  component ddfs_lut_4096_6bit is
    port (
      address  : in std_logic_vector(11 downto 0);
      ddfs_out : out std_logic_vector(5 downto 0)
      );
  end component;

begin

  PHASE_ACCUMULATOR: Counter
    generic map (N => 12)
    port map (
      clk     => clk,
      a_rst_h => reset,

      increment => fw,
      en        => '1',
      cntr_out  => phase_out
    );


  LUT_4096 : ddfs_lut_4096_6bit
    port map(
      address  => phase_out,
      ddfs_out => lut_output
    );

  DDFS_OUTPUT_REG: process(clk, reset)
  begin
    if (reset = '1') then
      output_reg <= (others => '0');
    elsif (rising_edge(clk)) then
      output_reg <= lut_output;
    end if;
  end process;

  yq <= output_reg;

end architecture;
