library IEEE;
use IEEE.std_logic_1164.all;

entity ddfs_wrapper is
  port (
    clk   : in std_logic;  -- clock of the system
    reset : in std_logic;  -- Asynchronous reset - active high

    fw      : in  std_logic_vector(3 downto 0);  -- input frequency word
    led_out : out std_logic_vector(3 downto 0);  -- input frequency word
    yq      : out std_logic_vector(5 downto 0)   -- output waveform
  );
end entity;

architecture struct of ddfs_wrapper is
  -------------------------------------------------------------------------------------
  -- Internal signals
  -------------------------------------------------------------------------------------

  -- Output of of the phase accumulator counter
  signal fw_internal : std_logic_vector(11 downto 0);
  -- Output of the LUT table
  signal ddfs_output : std_logic_vector(5 downto 0);

  -------------------------------------------------------------------------------------
  -- Internal Component
  -------------------------------------------------------------------------------------
  component DDFS is
    port (
      clk   : in std_logic;
      reset : in std_logic;

      fw : in std_logic_vector(11 downto 0);
      yq : out std_logic_vector(5 downto 0)
    );
  end component;

begin

    DDFS_i: DDFS
      port map (
        clk   => clk,
        reset => reset,

        fw => fw_internal,
        yq => ddfs_output
      );

    -- Frequency word construction
    fw_internal(3 downto 0)  <= fw;
    fw_internal(11 downto 4) <= (others => '0');

    -- Mapping the output
    yq      <= ( not(ddfs_output(5)) & ddfs_output(4 downto 0) );
    led_out <= fw;

end architecture;
