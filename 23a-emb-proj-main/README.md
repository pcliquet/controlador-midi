# Projeto Embarcados

##README

##FIRMWARE

PROTOCOLO DE 10 BITS + EOF ('X')

Exemplo: 0000000000X <------------------------ pacote

##BITS
  ###botões -- interrupt via callback/flag
  1° bit -> botão 1 (nota dó, C)
  2° bit -> botão 2 (nota ré, D)
  3° bit -> botão 3 (nota mi, E)
  4° bit -> botão 4 (nota fá, F)
  5° bit -> botão 5 (nota sol, G)
  6° bit -> botão 6 (nota lá, A)
  7° bit -> botão 7 (nota si, B)
  
  ###potenciômetro -- interrupt via AFEC
  Os três últimos bits dizem respeito ao valor lido do pontenciômetro:
  -Foi discretizado o range em 8 valores (3 bits) via little endian.
  
  000 - volume 0
  001 - volume 1
  010 - volume 2
  011 - volume 3
  100 - volume 4
  101 - volume 5
  110 - volume 6
  111 - volume 7
  
  ##ORDEM CERTA DE LER OS BITS DO POTÊNCIOMETRO:
  - 8° bit é o mais significativo, então ler da esquerda para a direita!

  
