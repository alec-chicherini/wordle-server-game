### wordle-server-game
```bash
git clone https://github.com/alec-chicherini/wordle-server-game.git
cd wordle-server-game
git submodule init
git submodule update
docker build --target=wordle_server_game_run . -t wordle-server-game-run
cd ~