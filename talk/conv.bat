for %%I in (*.XML) do (
  recode UTF-8..ISO646-DE 0<%%I 1>Z%%I
  tlkconv --fromxml Z%%I %%~nI.GER
  del Z%%I
)
