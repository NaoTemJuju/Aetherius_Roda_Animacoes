# Aetherius_Roda_Animações

Roda de gestos para Skyrim Special Edition/Anniversary Edition, com interface circular Meridian e plugin SKSE nativo. Esta implementação é **single player**, criada para testar a interface e executar gestos locais antes da adaptação ao multiplayer Aetherius.

## Controles

- **K**: abrir e fechar a roda.
- **Clique**: selecionar um gesto.
- **Esc / botão direito**: fechar.
- **Centro da roda**: interromper o gesto.
- **WASD / espaço / R**: interromper o gesto em execução.

São 45 opções em seis categorias. A interface usa ícones SVG desenhados em traços e cores discretas, sem painel retangular. Os gestos são eventos do grafo de animação do Skyrim; este projeto não distribui animações HKX novas. O gesto Ritual envia `IdleRitualStart`.

## Requisitos

Skyrim SE/AE para Windows x64; SKSE64 da versão do jogo; Address Library compatível; Meridian UI com API `Meridian.View/1`. VR não é suportado. O teste local foi realizado em AE 1.6.1170. Não precisa de Skyrim Platform, cliente Aetherius ou conexão com servidor.

## Compilar

Use Visual Studio 2022 Build Tools (MSVC), CMake 3.24+, Ninja, CommonLib NG de alandtse/CommonLibVR branch `ng` e dependências vcpkg `x64-windows-static-md`: spdlog, fmt, directxtk e nlohmann-json. A biblioteca CommonLib deve corresponder aos headers e ser compilada Release, SE+AE, /MD, C++23. A compilação local usou a revisão `5decf47b01dde5501b03afaa91cd4d182e793cca` do CommonLib.

Em um terminal de desenvolvimento x64 do Visual Studio:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCOMMONLIB_INCLUDE="D:/deps/CommonLibSSE-NG/include" -DCOMMONLIB_LIBRARY="D:/deps/CommonLibSSE.lib" -DDEPENDENCIES="D:/deps/vcpkg/installed/x64-windows-static-md"
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
cmake --install build --prefix dist
```

Os caminhos acima são exemplos. O projeto não baixa nem compila CommonLib automaticamente. O SDK público Meridian usado pelo consumidor está em `third_party/MeridianUIAPI`, com sua licença MIT.

## Instalar pelo MO2

Depois de compilar e instalar com CMake, crie um mod contendo o conteúdo de `dist/`:

```text
SKSE/Plugins/EmotesSP.dll
MeridianUI/emotes-sp/index.html
MeridianUI/emotes-sp/bridge.js
MeridianUI/emotes-sp/emotes.js
MeridianUI/emotes-sp/emotes.css
MeridianUI/emotes-sp/sketches.js
```

Ative o mod e Meridian UI em um perfil single player. Execute o jogo pelo SKSE, carregue um save, saia de menus/combate/montaria/mobiliário e pressione K. O plugin força terceira pessoa para mostrar a animação.

O perfil de teste não deve carregar o cliente multiplayer antigo. Verifique também DLLs instaladas diretamente no `Data/SKSE/Plugins` do jogo: desabilitar um mod no MO2 não desabilita essa cópia física. Se `SkyrimPlatform.dll` continuar carregando a página `mod://aetherius-client/index.html`, ela pode cobrir o jogo com uma tela 404. Preserve o arquivo antes de desativá-lo e restaure-o antes de voltar ao multiplayer. Este repositório não altera a instalação nem os perfis do usuário automaticamente.

## Arquitetura e integração futura

- `src/plugin.cpp`: ciclo de vida SKSE, tecla K, transporte Meridian, validação de comandos e execução local no jogador.
- `src/session.h`: sessão da página, abertura, fechamento e foco Meridian; não fecha a roda apenas porque a aquisição de foco foi assíncrona.
- `src/catalog.h`: lista permitida de eventos de animação.
- `ui/`: interface, ícones e ponte JavaScript.
- `tests/session.cpp`: teste da sessão com provedor Meridian simulado.

A página usa a origem exclusiva `mod://emotes-sp/index.html`. A função vinculada `emotesSPMessage` transporta JSON com ações `ready`, `play` (com `id`), `stop`, `close` e `ui-error`. O código copia dados recebidos do CEF e agenda alterações no jogo pela task interface do SKSE. Só aceita IDs do catálogo, aplica intervalo entre gestos e bloqueia contextos inadequados.

**Multiplayer ainda não implementado:** não há sincronização, autorização de servidor nem execução em jogadores remotos. Na adaptação, separar a seleção visual da execução, validar catálogo e contexto no servidor e transmitir eventos aos clientes pelo transporte real do Aetherius. `Play`/`Start` são os pontos atuais de execução local; não constituem um adaptador MP pronto.

## Validação e limitações

Compilação Release concluída e teste da sessão aprovado. Em teste no jogo, K abriu o radial e gestos selecionados foram vistos e ouvidos; logs confirmaram aceitação de eventos de chamar atenção e trompas. A tela 404 observada veio de uma segunda página criada pelo cliente antigo, que foi desativado na instalação de teste. A ausência dessa tela após a correção ainda precisa de novo teste. Não foram validados todos os 45 gestos.

Aceitação pelo grafo não garante movimento visível em todas as raças, esqueletos e modlists. O log `EmotesSP.log`, na pasta de logs SKSE, registra os eventos aceitos ou recusados. Não há persistência de gesto no save.

## Licença

Código do sistema: GPL-3.0-or-later (`LICENSE`). SDK Meridian: MIT, conforme `third_party/MeridianUIAPI/LICENSE-MIT`. Avisos do CommonLib em `licenses/`. Não inclui saves, backups, modlist, cliente/servidor Aetherius completo, binaries de dependências ou arquivos comerciais do Skyrim.
