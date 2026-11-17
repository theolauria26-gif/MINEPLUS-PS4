# Contrato de integração do renderer voxel

Este documento define a próxima etapa técnica do renderer nativo. Ele não afirma que a renderização GPU já existe: o cliente jogável continua usando SDL2 com raycasting/height-field, enquanto os módulos de malha permanecem no lado CPU.

## Estado dos dados existente

- `VoxelMesher` produz quads greedy com material e face.
- `VoxelMeshVertexBuilder` converte cada quad em quatro vértices com posição, UV local, material e normal axial.
- `material_atlas_region` resolve IDs válidos para células do atlas 8×4, com margem de meio texel.
- `WorldRuntime` e `ProceduralVoxelChunk` fornecem dados de chunks e edições, mas ainda não alimentam um renderer GPU do loop principal.

## Contrato mínimo do backend

A futura camada de renderização deve ser independente do gerador procedural e possuir operações explícitas:

1. Inicializar o dispositivo e validar as capacidades necessárias.
2. Criar/atualizar um recurso de malha a partir de vértices CPU e índices.
3. Associar material/atlas sem depender de caminhos de arquivo implícitos.
4. Desenhar uma lista limitada de malhas visíveis com matriz de câmera e parâmetros de iluminação.
5. Destruir recursos de forma determinística antes de encerrar o renderer.

O backend deve receber dados já construídos. Nenhuma chamada SDL, API gráfica ou upload de GPU pode ocorrer dentro do gerador de chunks ou do worker de geração.

## Formato de vértices e índices

O contrato CPU atual usa posição XYZ em unidades de bloco, UV local repetível, ID de material e normal axial. O backend deve:

- Preservar UV local até o shader aplicar repetição dentro da célula do atlas.
- Resolver o ID de material usando a região do atlas; não interpolar diretamente entre células vizinhas.
- Usar amostragem nearest-neighbor e mipmapping desativado inicialmente, até existir atlas com gutters e testes de bleeding.
- Gerar dois triângulos por quad com winding consistente com a ordem CCW documentada pelo builder.
- Validar índices, limites de buffer e IDs de material antes do upload.

## Ciclo de vida e estabilidade

- Cada chunk possui uma versão de geometria. Uma edição invalida somente os chunks afetados e seus vizinhos quando altera faces de fronteira.
- Uploads e destruição de recursos acontecem na thread do renderer.
- Geração de voxels, meshing e preparação de vértices podem ocorrer fora dela, usando buffers CPU próprios.
- A fila de uploads tem orçamento por frame; chunks pendentes permanecem visíveis pela malha anterior ou por fallback, nunca por memória parcialmente atualizada.
- Ao descarregar um chunk, remover primeiro sua referência da lista visível e liberar o recurso na thread correta.
- Falha de alocação deve manter o jogo executável e registrar o chunk como pendente para nova tentativa limitada.

## Plano de integração em incrementos verificáveis

1. Implementar backend de prova de conceito sem trocar o renderer ativo.
2. Desenhar um chunk isolado com atlas de teste e câmera fixa.
3. Validar as seis orientações de face, winding, UV repetida, normais e limites de material.
4. Adicionar fila de upload e invalidação por versão, incluindo fronteiras de chunks.
5. Integrar câmera, colisão e edição em uma área de teste separada.
6. Só então habilitar o renderer voxel no fluxo jogável, mantendo fallback até os testes passarem.

## Critérios de aceite

- Testes CPU cobrem material inválido, quad degenerado, overflow de vértices, winding e UVs.
- Teste gráfico mostra todas as seis faces sem culling incorreto ou vazamento entre tiles.
- Mineração/colocação atualiza a malha correta sem duplicar faces internas.
- Streaming não chama API gráfica fora da thread de renderização.
- Falha de upload ou falta de memória não encerra o jogo nem corrompe a malha anterior.
- Build automatizada passa; FPS, memória e estabilidade ainda precisam ser medidos em hardware PS4 real.

## Limitações conhecidas

Não há nesta etapa shader, textura de atlas final, buffers GPU integrados ao loop principal nem validação visual em PS4. Este contrato é uma especificação de implementação para evitar que a integração futura misture geração procedural, gerenciamento de recursos gráficos e gameplay em uma única camada.
