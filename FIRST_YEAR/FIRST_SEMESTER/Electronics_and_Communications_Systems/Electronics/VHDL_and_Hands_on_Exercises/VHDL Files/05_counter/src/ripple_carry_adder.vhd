library IEEE;
  use IEEE.std_logic_1164.all;

entity ripple_carry_adder is
  generic ( Nbit : positive := 8);
  port (
    a    : in std_logic_vector(Nbit - 1 downto 0);
    b    : in std_logic_vector(Nbit - 1 downto 0);
    cin  : in std_logic;
    s    : out std_logic_vector (Nbit - 1 downto 0);
    cout : out std_logic
  );
end entity;

architecture beh of ripple_carry_adder is

  component full_adder is
    port (
      a    : in  std_logic;
      b    : in  std_logic;
      cin  : in  std_logic;
      s    : out std_logic;
      cout : out std_logic
    );
  end component;

  signal carry : std_logic_vector (Nbit - 1 downto 0);

begin

  GEN: for i in 0 to Nbit-1 generate

    FIRST: if i = 0 generate
      FA_1: full_adder
        port map (
          a    => a(i),
          b    => b(i),
          cin  => cin,
          s    => s(i),
          cout => carry(i)
        );
    end generate;

    SECONDS: if i > 0 and i < Nbit generate
      FA_M: full_adder
        port map (
          a    => a(i),
          b    => b(i),
          cin  => carry(i-1),
          s    => s(i),
          cout => carry(i)
        );
    end generate;

  end generate;

  cout <= carry(Nbit-1);

end architecture;
