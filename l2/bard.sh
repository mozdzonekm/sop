#!/bin/bash

if [ $# -ne 2 ]; then
  echo "Usage: $0 <fifo> <records>" >&2
  exit 1
fi

bodies=(
  "Preheat the oven to 180 degrees"
  "Line three cake tins with baking paper"
  "Separate the yolks from the whites of twelve eggs"
  "Beat the egg whites stiff with a pinch of salt"
  "Gradually add sugar while still beating"
  "Add the yolks one at a time, mixing constantly"
  "Sift wheat flour together with potato starch"
  "Gently fold in the flour with a spatula"
  "Pour the batter into the tins and level the tops"
  "Bake for 35 minutes until a skewer comes out dry"
  "Cool the sponges and slice each one in half"
  "Boil water with lemon juice, sugar and a few raw~berry"
  "Cream the butter until light and fluffy"
  "Mash half of the raw~berry, keep the other raw~berry whole"
  "Fold in mascarpone and seeds from a vanilla pod"
  "Divide the cream into three equal parts"
  "Add melted dark chocolate to the first part"
  "Add pureed raw~berry to the second part"
  "Leave the third part white, princess Joanna loves it the most"
  "Soak the first sponge layer with syrup"
  "Spread it with the chocolate cream"
  "Cover with the next sponge layer and soak it"
  "Spread the raw~berry cream and lay fresh raw~berry on top"
  "Repeat the layers until all the sponges are used"
  "Chill the cake overnight in the castle cellar"
  "Cover the sides with white cream and press in raw~berry"
  "Melt sugar in a saucepan into golden caramel"
  "Shape small crowns out of the caramel and raw~berry"
  "Sprinkle the top with edible gold and crushed raw~berry"
  "Place the crowns on top, surrounded by raw~berry"
  "Put three candles in the cake"
  "Serve with tea, the bards get the biggest slices"
)

fifo=$1
n=$2

if ! [[ $n =~ ^[0-9]+$ ]]; then
  echo "records must be a non-negative number" >&2
  exit 1
fi

if [ -e "$fifo" ] || [ -p "$fifo" ]; then
  echo "FIFO $fifo already exists. Another bard may be using it." >&2
  exit 1
fi

if ! mkfifo "$fifo"; then
  echo "Cannot create named pipe $fifo" >&2
  exit 1
fi

# remove the pipe if script ends
trap 'rm -f "$fifo"' EXIT
trap 'exit 130' INT TERM HUP QUIT
# ignore SIGPIPE
trap '' PIPE

echo "Bard from $fifo is waiting for the kitchen to listen..."

exec {fd}>"$fifo"

echo "Bard from $fifo starts dictating"

for ((i = 0; i < n; i++)); do
  body=${bodies[RANDOM % ${#bodies[@]}]}

  # bard sometimes mixes up the line number
  if ((RANDOM % 8 == 0)); then
    num=$((RANDOM % 68 + 32))
  else
    num=$((RANDOM % 32))
  fi

  # truncate to 63 characters
  if ((${#body} > 63)); then
    body=${body:0:63}
  fi

  # pad to exactly 63 characters
  printf -v bodypad '%-63s' "$body"

  # two-digit number
  printf -v numstr '%02d' "$num"

  # build a 67-byte record:
  # NN: + 63 bytes + \n
  msg="${numstr}:${bodypad}"$'\n'

  # random corruption, preserving the 67-byte length
  case $((RANDOM % 10)) in
  0)
    # wrong separator
    msg="${msg:0:2}?${msg:3}"
    ;;
  1)
    # wrong terminator
    msg="${msg:0:66};"
    ;;
  esac

  # print to bard's terminal
  printf '%s\n' "${msg%$'\n'}"

  # send the record in random pieces
  pos=0
  total=${#msg}
  pieces=$((RANDOM % 4 + 1))

  while ((pos < total)); do
    if ((pieces > 1 && total - pos > 1)); then
      len=$((RANDOM % (total - pos) + 1))
    else
      len=$((total - pos))
    fi

    if ! printf '%s' "${msg:pos:len}" >&"$fd"; then
      break 2
    fi

    pos=$((pos + len))

    if ((pos < total)); then
      sleep 0.$((RANDOM % 9 + 1))
    fi

    pieces=$((pieces - 1))
  done

  # random pause between records
  if ((RANDOM % 5 == 0)); then
    sleep 1
  else
    sleep 0.$((RANDOM % 9 + 1))
  fi
done

exec {fd}>&-

if ((i < n)); then
  echo "The kitchen stopped listening, bard from $fifo goes back" >&2
  exit 1
fi

echo "Bard from $fifo has finished dictating"
