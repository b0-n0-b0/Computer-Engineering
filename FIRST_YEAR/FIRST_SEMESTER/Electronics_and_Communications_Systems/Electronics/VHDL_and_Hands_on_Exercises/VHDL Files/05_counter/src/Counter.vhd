library ieee;
  use ieee.std_logic_1164.all;

entity Counter is
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

end entity;

architecture struct of Counter is
  --------------------------------------------------------------
  -- Signals declaration
  --------------------------------------------------------------

  -- Output of the fullAdder_N
  signal fullAdder_out : std_logic_vector(N - 1 downto 0);

  -- Output of the DFF_N
  signal q_h : std_logic_vector(N - 1 downto 0);


  --------------------------------------------------------------
  -- Components declaration
  --------------------------------------------------------------

  component DFF_N is
    generic( N : natural := 8);

    port(
      clk     : in std_logic;
      a_rst_n : in std_logic;
      en      : in std_logic;
      d       : in std_logic_vector(N - 1 downto 0);
      q       : out std_logic_vector(N - 1 downto 0)
    );
  end component;

  component ripple_carry_adder is
    generic (Nbit : integer := 8);

    port (
      a    : in std_logic_vector(Nbit - 1 downto 0);
      b    : in std_logic_vector(Nbit - 1 downto 0);
      cin  : in std_logic;
      s    : out std_logic_vector (Nbit - 1 downto 0);
      cout : out std_logic
    );
  end component;

begin

  FULL_ADDER_N_MAP : ripple_carry_adder
    generic map (Nbit => N)
    port map (
      a    => increment,
      b    => q_h,
      cin  => '0',
      s    => fullAdder_out,
      cout => open
    );

  DFF_N_MAP : DFF_N
    generic map (N => N)
    port map (
      clk     => clk,
      a_rst_n => a_rst_n,
      d       => fullAdder_out,
      en      => en,
      q       => q_h
    );

  -- Connect the output
  cntr_out <= q_h;

end architecture;
