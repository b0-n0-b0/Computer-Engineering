library ieee;
  use ieee.std_logic_1164.all;

entity fifo is
  generic (
    DEPTH      : natural := 4;
    DATA_WIDTH : natural := 8
  );
  port(
    clk      : in std_logic;
    a_rst_n  : in std_logic;
    data_in  : in std_logic_vector(DATA_WIDTH - 1 downto 0);
    data_out : out std_logic_vector(DATA_WIDTH - 1 downto 0)
  );
end entity;

architecture struct of fifo is
  --------------------------------------------------------------
  -- Signals declaration
  --------------------------------------------------------------
  type internal_fifo_signal is array (0 to DEPTH - 1) of std_logic_vector(DATA_WIDTH - 1 downto 0);
  signal int_fifo : internal_fifo_signal;

  --------------------------------------------------------------
  -- Components declaration
  --------------------------------------------------------------
  component DFF_N is
    generic ( N : natural := 8 );
    port (
      clk     : in std_logic;
      a_rst_n : in std_logic;
      en      : in std_logic;
      d       : in std_logic_vector(N - 1 downto 0);
      q       : out std_logic_vector(N - 1 downto 0)
    );
  end component;

begin

  GEN: for i in 0 to DEPTH - 1 generate

    FIRST: if i = 0 generate
      DDF_N_1: DFF_N
        generic map ( N  => DATA_WIDTH )
        port map (
          clk     => clk,
          a_rst_n => a_rst_n,
          en      => '1',
          d       => data_in,
          q       => int_fifo(i)
        );
    end generate;

    SECONDS: if i > 0 and i < DEPTH generate
      DDF_N_2: DFF_N
        generic map ( N  => DATA_WIDTH )
        port map (
          clk     => clk,
          a_rst_n => a_rst_n,
          en      => '1',
          d       => int_fifo(i-1),
          q       => int_fifo(i)
        );
    end generate;

  end generate;

  -- Connect the output
  data_out <= int_fifo(DEPTH-1);

end architecture;
