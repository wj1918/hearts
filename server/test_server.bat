@echo off
REM Hearts AI Server Test Suite for Windows
REM Usage: test_server.bat [host:port]

set HOST=%1
if "%HOST%"=="" set HOST=localhost:8080

echo === Hearts AI Server Test Suite ===
echo Testing server at: %HOST%
echo.

echo Test 1: Health check
curl -s "http://%HOST%/api/health"
echo.
echo.

echo Test 2: Single card (2C) - should return 2C
curl -s -X POST "http://%HOST%/api/move" -H "Content-Type: application/json" -d "{\"game_state\":{\"player_hands\":[[{\"suit\":2,\"rank\":12}],[],[],[]],\"current_player\":0,\"current_trick\":{\"cards\":[],\"lead_player\":0},\"played_cards\":[[],[],[],[]],\"scores\":[0,0,0,0],\"hearts_broken\":false},\"ai_config\":{\"simulations\":100}}"
echo.
echo.

echo Test 3: Multiple clubs (100 simulations)
curl -s -X POST "http://%HOST%/api/move" -H "Content-Type: application/json" -d "{\"game_state\":{\"player_hands\":[[{\"suit\":2,\"rank\":0},{\"suit\":2,\"rank\":12}],[{\"suit\":2,\"rank\":1},{\"suit\":2,\"rank\":4}],[{\"suit\":2,\"rank\":2},{\"suit\":2,\"rank\":8}],[{\"suit\":2,\"rank\":3},{\"suit\":2,\"rank\":10}]],\"current_player\":0,\"current_trick\":{\"cards\":[],\"lead_player\":0},\"played_cards\":[[],[],[],[]],\"scores\":[0,0,0,0],\"hearts_broken\":false},\"ai_config\":{\"simulations\":100}}"
echo.
echo.

echo Test 4: Higher simulations (1000)
curl -s -X POST "http://%HOST%/api/move" -H "Content-Type: application/json" -d "{\"game_state\":{\"player_hands\":[[{\"suit\":2,\"rank\":0},{\"suit\":2,\"rank\":12}],[{\"suit\":2,\"rank\":1},{\"suit\":2,\"rank\":4}],[{\"suit\":2,\"rank\":2},{\"suit\":2,\"rank\":8}],[{\"suit\":2,\"rank\":3},{\"suit\":2,\"rank\":10}]],\"current_player\":0,\"current_trick\":{\"cards\":[],\"lead_player\":0},\"played_cards\":[[],[],[],[]],\"scores\":[0,0,0,0],\"hearts_broken\":false},\"ai_config\":{\"simulations\":1000}}"
echo.
echo.

echo Test 5: Full 13 cards per player
curl -s -X POST "http://%HOST%/api/move" -H "Content-Type: application/json" -d "{\"game_state\":{\"player_hands\":[[{\"suit\":2,\"rank\":0},{\"suit\":2,\"rank\":1},{\"suit\":2,\"rank\":2},{\"suit\":2,\"rank\":3},{\"suit\":2,\"rank\":4},{\"suit\":2,\"rank\":5},{\"suit\":2,\"rank\":6},{\"suit\":2,\"rank\":7},{\"suit\":2,\"rank\":8},{\"suit\":2,\"rank\":9},{\"suit\":2,\"rank\":10},{\"suit\":2,\"rank\":11},{\"suit\":2,\"rank\":12}],[{\"suit\":0,\"rank\":0},{\"suit\":0,\"rank\":1},{\"suit\":0,\"rank\":2},{\"suit\":0,\"rank\":3},{\"suit\":0,\"rank\":4},{\"suit\":0,\"rank\":5},{\"suit\":0,\"rank\":6},{\"suit\":0,\"rank\":7},{\"suit\":0,\"rank\":8},{\"suit\":0,\"rank\":9},{\"suit\":0,\"rank\":10},{\"suit\":0,\"rank\":11},{\"suit\":0,\"rank\":12}],[{\"suit\":1,\"rank\":0},{\"suit\":1,\"rank\":1},{\"suit\":1,\"rank\":2},{\"suit\":1,\"rank\":3},{\"suit\":1,\"rank\":4},{\"suit\":1,\"rank\":5},{\"suit\":1,\"rank\":6},{\"suit\":1,\"rank\":7},{\"suit\":1,\"rank\":8},{\"suit\":1,\"rank\":9},{\"suit\":1,\"rank\":10},{\"suit\":1,\"rank\":11},{\"suit\":1,\"rank\":12}],[{\"suit\":3,\"rank\":0},{\"suit\":3,\"rank\":1},{\"suit\":3,\"rank\":2},{\"suit\":3,\"rank\":3},{\"suit\":3,\"rank\":4},{\"suit\":3,\"rank\":5},{\"suit\":3,\"rank\":6},{\"suit\":3,\"rank\":7},{\"suit\":3,\"rank\":8},{\"suit\":3,\"rank\":9},{\"suit\":3,\"rank\":10},{\"suit\":3,\"rank\":11},{\"suit\":3,\"rank\":12}]],\"current_player\":0,\"current_trick\":{\"cards\":[],\"lead_player\":0},\"played_cards\":[[],[],[],[]],\"scores\":[0,0,0,0],\"hearts_broken\":false},\"ai_config\":{\"simulations\":100}}"
echo.
echo.

echo Test 6: Mid-game with played cards
curl -s -X POST "http://%HOST%/api/move" -H "Content-Type: application/json" -d "{\"game_state\":{\"player_hands\":[[{\"suit\":2,\"rank\":0},{\"suit\":2,\"rank\":1}],[{\"suit\":2,\"rank\":2},{\"suit\":2,\"rank\":3}],[{\"suit\":2,\"rank\":4},{\"suit\":2,\"rank\":5}],[{\"suit\":2,\"rank\":6},{\"suit\":2,\"rank\":7}]],\"current_player\":0,\"current_trick\":{\"cards\":[],\"lead_player\":0},\"played_cards\":[[{\"suit\":2,\"rank\":12}],[{\"suit\":2,\"rank\":11}],[{\"suit\":2,\"rank\":10}],[{\"suit\":2,\"rank\":9}]],\"scores\":[0,0,0,0],\"hearts_broken\":false},\"ai_config\":{\"simulations\":500}}"
echo.
echo.

echo === All tests completed ===
