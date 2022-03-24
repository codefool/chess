# chess
Research to generate all possible games of Chess

Maximum possible moves for any given position
```
 K -  8 * 1 =  8
 Q - 28 * 1 = 28 (R+B, so can use this in determining poss. moves for Q)
 R - 14 * 2 = 56
 N -  8 * 2 = 16
 B - 13 * 2 = 26
 P -  1 * 8 =  8
             142
```

Pack in a byte. Hi nibble is piece id 0-31, and lo nibble is target square.
If target square is occupied capture is inferred.
```
xxxx .... .... .... - action
.... xxxx xx.. .... - Source square
.... .... ..xx xxxx - Target square

Action:
0x0000 moves
0x0001 captures
0x0010 castle kingside
0x0011 castle queenside
0x0100 check
0x0101 checkmate
0x0110 en passant
0x0111 unused
0x1000 promotion queen
0x1001 promotion bishop
0x1010 promotion knight
0x1011 promotion rook
0x1100 unused
0x1101 unused
0x1110 unused
0x1111 unused
```
Pieces are packed in nibbles of a 32-byte array, as follows:
```
x... - side 0=white, 1=black
.xxx - piece type
.001 - King
.010 - Queen
.011 - Bishop
.100 - Knight
.101 - Rook
.110 - Pawn
.111 - Pawn not in its own file
0000 - empty square
```
Each nibble of the position array represents one square on the chessboard. This is
more efficient than encoding location data per-piece, and also allows easy
addition of duplicate piece types through pawn promotion.
```
xxxx x... .... .... .... .... .... .... = number of active pieces on the board (0..31)
.... .x.. .... .... .... .... .... .... = side on move: 0-white, 1-black
.... ..xx .... .... .... .... .... .... = drawn game reason
.... .... x... .... .... .... .... .... = white castle kingside disabled  (WK or WKR has moved)
.... .... .x.. .... .... .... .... .... = white castle queenside disabled (WK or WQR has moved)
.... .... ..x. .... .... .... .... .... = black castle kingside disabled  (BK or BKR has moved)
.... .... ...x .... .... .... .... .... = black castle queenside disabled (BK or BQR has moved)
.... .... .... x... .... .... .... .... = en passant latch
.... .... .... .xxx .... .... .... .... = pawn on file xxx is subject to en passant
.... .... .... .... x... .... .... .... = drawn game
.... .... .... .... .xx. .... .... .... = 14D reason
.... .... .... .... ...? ???? ???? ???? = unused (zeroes)
```
It is imperitive that all unused bits - or bits that are out of scope - be set to 0

Which pieces do we need to know if they have moved or not?
- if pawns are on their home square, then they have not moved (since they can only move forward)
- en passant restrictions:
  - the capturing pawn must have advanced exactly three ranks
  - the captured pawn must have moved two squares in one move
  - the en passant capture must be performed on the turn immediately after the pawn being
    captured moves, otherwise it is illegal.

  This is handled through two mechanisms:
   - When a pawn moves off its own file (via capture), its type is changed to PT_PAWN_O.
   - When a pawn advances two spaces on its first move, en_passant_latch is set to 1, and en_passant_file
     records the file where the pawn rests. On the subsequent position(s), the latch and file are reset to
     zero.
   This works because there can be at most one pawn subject to en passant, but multiple opponent pawns that
   could affect en passant on that pawn. If en passant is affected, it must be done on the VERY NEXT MOVE
   by the opponent, hence its not a permanent condition we need to maintain. For a pawn to affect en passant,
   however, if can not have moved off its own file and must have advanced exactly three ranks - but not
   necessarilly on consecutive turns. So, we have to record the fact that a pawn has changed files.

- castle restrictions:
  - king and rook have not moved
  - king is not in check (cannot castle out of check)
  - king does not pass through check
  - no pieces between the king and rook

The "order" of a position is the number of pieces in the position. Since chess games
must naturally lose material as the game progresses, then once all possible edges from
P(32) are known, then only those less than P(32) remain. So, we exhaust all P(32) positions,
which will generate some number of P(31) positions. Then repeat with P(31) all the way down theoretically to P(2), but drawn games will be determined before then. 

P(32) - initial position
    move 1 - 20 white moves,  21 total positions, 0 collisions
    move 2 - 20 black moves, 401 total positions, 0 collisions
    move 3 - 

For each position P:
1. Marked P as WIP
2. Determine the complete list of legal moves for the side on-move.
3. For each move
   a. Generate a new position P'
   b. Check if P' exists.
   c. If not, add to table with state UNRESOLVED.
4. Mark P as RESOLVED

If there are no moves for a given position, then it is a terminating position:
- if the on-side king is in check, it is checkmate
- otherwise it is stalemate

PAWNS CANNOT RETREAT

The reason cycles occur is that pieces can retreat back to their original position
in most cases, but pawns cannot.


Completed first run of defering pawn moves, and the algorithm completed with the following:
Total number of pawn-move positions: 324'956'199
                resolved  positions:  25'452'174
                n-1       positions:  55'112'931
                unique    positions: 405'521'304

The total number of positions generated, however, is 582'301'835 which suggests there is a
great inefficiency in the algorithm.


139704283731712 base:571568980 moves:23 src:183529064 mov:1:c5b3 I:324955399 R:25452124 U:50 U1:55112745 r1bqkbr1/pppppppp/8/8/8/NN6/PPPPPPPP/RnBQKBRn b q - 0 0
139704283731712 base:571569005 moves:24 src:183529087 mov:1:c5b3 I:324955415 R:25452125 U:49 U1:55112749 r1bqkbr1/pppppppp/8/8/8/nN6/PPPPPPPP/nRBQKBRN b q - 0 0
139704283731712 base:571569030 moves:23 src:183529109 mov:1:c5b3 I:324955431 R:25452126 U:48 U1:55112751 r1bqkbr1/pppppppp/8/8/8/nN6/PPPPPPPP/RnBQKBRN b q - 0 0
139704283731712 base:571569056 moves:24 src:183529132 mov:1:c5b3 I:324955447 R:25452127 U:47 U1:55112755 r1bqkbr1/pppppppp/8/8/8/nN6/PPPPPPPP/nRBQKBNR b q - 0 0
139704283731712 base:571569082 moves:23 src:183529154 mov:1:c5b3 I:324955463 R:25452128 U:46 U1:55112757 r1bqkbr1/pppppppp/8/8/8/nN6/PPPPPPPP/RnBQKBNR b q - 0 0
139704283731712 base:571569108 moves:25 src:183529178 mov:1:c5b3 I:324955479 R:25452129 U:45 U1:55112760 r1bqkbr1/pppppppp/8/8/8/nN6/PPPPPPPP/RNBQKBnR b q - 0 0
139704283731712 base:571569133 moves:25 src:183529202 mov:1:c5b3 I:324955495 R:25452130 U:44 U1:55112763 r1bqkbr1/pppppppp/8/8/8/nN6/PPPPPPPP/NRBQKBnR b q - 0 0
139704283731712 base:571569159 moves:24 src:183529225 mov:1:c5b3 I:324955511 R:25452131 U:43 U1:55112766 r1bqkbr1/pppppppp/8/8/8/nN6/PPPPPPPP/RNBQKBRn b q - 0 0
139704283731712 base:571569184 moves:24 src:183529248 mov:1:c5b3 I:324955527 R:25452132 U:42 U1:55112769 r1bqkbr1/pppppppp/8/8/8/nN6/PPPPPPPP/NRBQKBRn b q - 0 0
139704283731712 base:573828438 moves:24 src:185786898 mov:1:b5a3 I:324955543 R:25452133 U:41 U1:55112772 r1bqkbr1/pppppppp/8/8/8/Nn6/PPPPPPPP/nRBQKBRN b q - 0 0
139704283731712 base:573828462 moves:27 src:185786924 mov:1:b5a3 I:324955559 R:25452134 U:40 U1:55112777 r1bqkbr1/pppppppp/8/8/8/Nn6/PPPPPPPP/RnBQKBRN b q - 0 0
139704283731712 base:573828487 moves:24 src:185786947 mov:1:b5a3 I:324955575 R:25452135 U:39 U1:55112780 r1bqkbr1/pppppppp/8/8/8/Nn6/PPPPPPPP/nRBQKBNR b q - 0 0
139704283731712 base:573828512 moves:27 src:185786973 mov:1:b5a3 I:324955591 R:25452136 U:38 U1:55112785 r1bqkbr1/pppppppp/8/8/8/Nn6/PPPPPPPP/RnBQKBNR b q - 0 0
139704283731712 base:573828537 moves:27 src:185786999 mov:1:b5a3 I:324955607 R:25452137 U:37 U1:55112789 r1bqkbr1/pppppppp/8/8/8/Nn6/PPPPPPPP/RNBQKBnR b q - 0 0
139704283731712 base:573828561 moves:27 src:192989172 mov:1:b5a3 I:324955623 R:25452138 U:36 U1:55112793 r1bqkbr1/pppppppp/8/8/8/Nn6/PPPPPPPP/NRBQKBnR b q - 0 0
139704283731712 base:573828586 moves:26 src:185787024 mov:1:b5a3 I:324955639 R:25452139 U:35 U1:55112797 r1bqkbr1/pppppppp/8/8/8/Nn6/PPPPPPPP/RNBQKBRn b q - 0 0
139704283731712 base:573828610 moves:26 src:192989244 mov:1:b5a3 I:324955655 R:25452140 U:34 U1:55112801 r1bqkbr1/pppppppp/8/8/8/Nn6/PPPPPPPP/NRBQKBRn b q - 0 0
139704283731712 base:582300119 moves:28 src:194256430 mov:1:g3h1 I:324955671 R:25452141 U:33 U1:55112806 r1bqkbr1/pppppppp/8/8/8/nn6/PPPPPPPP/RNBQKBRN b q - 0 0
139704283731712 base:582300138 moves:28 src:194256458 mov:1:g3h1 I:324955687 R:25452142 U:32 U1:55112811 r1bqkbr1/pppppppp/8/8/8/nn6/PPPPPPPP/NRBQKBRN b q - 0 0
139704283731712 base:582201543 moves:28 src:194156284 mov:1:h3g1 I:324955703 R:25452143 U:31 U1:55112816 r1bqkbr1/pppppppp/8/8/8/nn6/PPPPPPPP/RNBQKBNR b q - 0 0
139704283731712 base:582201561 moves:28 src:194156312 mov:1:h3g1 I:324955719 R:25452144 U:30 U1:55112821 r1bqkbr1/pppppppp/8/8/8/nn6/PPPPPPPP/NRBQKBNR b q - 0 0
139704283731712 base:571569430 moves:23 src:183529582 mov:1:c5b3 I:324955735 R:25452145 U:29 U1:55112824 r1bqkbr1/pppppppp/8/8/8/NN6/PPPPPPPP/nRBQKBnR b Kq - 0 0
139704283731712 base:571569457 moves:24 src:183529605 mov:1:c5b3 I:324955751 R:25452146 U:28 U1:55112827 r1bqkbr1/pppppppp/8/8/8/NN6/PPPPPPPP/RnBQKBnR b Kq - 0 0
139704283731712 base:571569483 moves:24 src:183529628 mov:1:c5b3 I:324955767 R:25452147 U:27 U1:55112831 r1bqkbr1/pppppppp/8/8/8/nN6/PPPPPPPP/nRBQKBNR b Kq - 0 0
139704283731712 base:571569509 moves:23 src:183529650 mov:1:c5b3 I:324955783 R:25452148 U:26 U1:55112833 r1bqkbr1/pppppppp/8/8/8/nN6/PPPPPPPP/RnBQKBNR b Kq - 0 0
139704283731712 base:571569535 moves:25 src:183529674 mov:1:c5b3 I:324955799 R:25452149 U:25 U1:55112836 r1bqkbr1/pppppppp/8/8/8/nN6/PPPPPPPP/RNBQKBnR b Kq - 0 0
139704283731712 base:571569560 moves:25 src:183529698 mov:1:c5b3 I:324955815 R:25452150 U:24 U1:55112839 r1bqkbr1/pppppppp/8/8/8/nN6/PPPPPPPP/NRBQKBnR b Kq - 0 0
139704283731712 base:573828918 moves:24 src:185787388 mov:1:b5a3 I:324955831 R:25452151 U:23 U1:55112842 r1bqkbr1/pppppppp/8/8/8/Nn6/PPPPPPPP/nRBQKBNR b Kq - 0 0
139704283731712 base:573828943 moves:27 src:185787414 mov:1:b5a3 I:324955847 R:25452152 U:22 U1:55112847 r1bqkbr1/pppppppp/8/8/8/Nn6/PPPPPPPP/RnBQKBNR b Kq - 0 0
139704283731712 base:573828968 moves:27 src:185787440 mov:1:b5a3 I:324955863 R:25452153 U:21 U1:55112851 r1bqkbr1/pppppppp/8/8/8/Nn6/PPPPPPPP/RNBQKBnR b Kq - 0 0
139704283731712 base:573828992 moves:27 src:192989630 mov:1:b5a3 I:324955879 R:25452154 U:20 U1:55112855 r1bqkbr1/pppppppp/8/8/8/Nn6/PPPPPPPP/NRBQKBnR b Kq - 0 0
139704283731712 base:582201764 moves:28 src:194156595 mov:1:h3g1 I:324955895 R:25452155 U:19 U1:55112860 r1bqkbr1/pppppppp/8/8/8/nn6/PPPPPPPP/RNBQKBNR b Kq - 0 0
139704283731712 base:582201782 moves:28 src:194156623 mov:1:h3g1 I:324955911 R:25452156 U:18 U1:55112865 r1bqkbr1/pppppppp/8/8/8/nn6/PPPPPPPP/NRBQKBNR b Kq - 0 0
139704283731712 base:571569699 moves:24 src:183529878 mov:1:c5b3 I:324955927 R:25452157 U:17 U1:55112868 r1bqkbr1/pppppppp/8/8/8/NN6/PPPPPPPP/RnBQKBnR b Qq - 0 0
139704283731712 base:571569726 moves:23 src:183529900 mov:1:c5b3 I:324955943 R:25452158 U:16 U1:55112871 r1bqkbr1/pppppppp/8/8/8/NN6/PPPPPPPP/RnBQKBRn b Qq - 0 0
139704283731712 base:571569751 moves:23 src:183529922 mov:1:c5b3 I:324955959 R:25452159 U:15 U1:55112873 r1bqkbr1/pppppppp/8/8/8/nN6/PPPPPPPP/RnBQKBRN b Qq - 0 0
139704283731712 base:571569777 moves:23 src:183529944 mov:1:c5b3 I:324955975 R:25452160 U:14 U1:55112875 r1bqkbr1/pppppppp/8/8/8/nN6/PPPPPPPP/RnBQKBNR b Qq - 0 0
139704283731712 base:571569803 moves:25 src:183529968 mov:1:c5b3 I:324955991 R:25452161 U:13 U1:55112878 r1bqkbr1/pppppppp/8/8/8/nN6/PPPPPPPP/RNBQKBnR b Qq - 0 0
139704283731712 base:571569829 moves:24 src:183529991 mov:1:c5b3 I:324956007 R:25452162 U:12 U1:55112881 r1bqkbr1/pppppppp/8/8/8/nN6/PPPPPPPP/RNBQKBRn b Qq - 0 0
139704283731712 base:573829184 moves:27 src:185787660 mov:1:b5a3 I:324956023 R:25452163 U:11 U1:55112886 r1bqkbr1/pppppppp/8/8/8/Nn6/PPPPPPPP/RnBQKBRN b Qq - 0 0
139704283731712 base:573829209 moves:27 src:185787686 mov:1:b5a3 I:324956039 R:25452164 U:10 U1:55112891 r1bqkbr1/pppppppp/8/8/8/Nn6/PPPPPPPP/RnBQKBNR b Qq - 0 0
139704283731712 base:573829234 moves:27 src:185787712 mov:1:b5a3 I:324956055 R:25452165 U:9 U1:55112895 r1bqkbr1/pppppppp/8/8/8/Nn6/PPPPPPPP/RNBQKBnR b Qq - 0 0
139704283731712 base:573829259 moves:26 src:185787737 mov:1:b5a3 I:324956071 R:25452166 U:8 U1:55112899 r1bqkbr1/pppppppp/8/8/8/Nn6/PPPPPPPP/RNBQKBRn b Qq - 0 0
139704283731712 base:582300311 moves:28 src:194256705 mov:1:g3h1 I:324956087 R:25452167 U:7 U1:55112904 r1bqkbr1/pppppppp/8/8/8/nn6/PPPPPPPP/RNBQKBRN b Qq - 0 0
139704283731712 base:582201946 moves:28 src:194156858 mov:1:h3g1 I:324956103 R:25452168 U:6 U1:55112909 r1bqkbr1/pppppppp/8/8/8/nn6/PPPPPPPP/RNBQKBNR b Qq - 0 0
139704283731712 base:571569968 moves:24 src:183530171 mov:1:c5b3 I:324956119 R:25452169 U:5 U1:55112912 r1bqkbr1/pppppppp/8/8/8/NN6/PPPPPPPP/RnBQKBnR b KQq - 0 0
139704283731712 base:571569994 moves:23 src:183530193 mov:1:c5b3 I:324956135 R:25452170 U:4 U1:55112914 r1bqkbr1/pppppppp/8/8/8/nN6/PPPPPPPP/RnBQKBNR b KQq - 0 0
139704283731712 base:571570020 moves:25 src:183530217 mov:1:c5b3 I:324956151 R:25452171 U:3 U1:55112917 r1bqkbr1/pppppppp/8/8/8/nN6/PPPPPPPP/RNBQKBnR b KQq - 0 0
139704283731712 base:573829427 moves:27 src:185787935 mov:1:b5a3 I:324956167 R:25452172 U:2 U1:55112922 r1bqkbr1/pppppppp/8/8/8/Nn6/PPPPPPPP/RnBQKBNR b KQq - 0 0
139704283731712 base:573829452 moves:27 src:185787961 mov:1:b5a3 I:324956183 R:25452173 U:1 U1:55112926 r1bqkbr1/pppppppp/8/8/8/Nn6/PPPPPPPP/RNBQKBnR b KQq - 0 0
139704283731712 base:582202058 moves:28 src:194157015 mov:1:h3g1 I:324956199 R:25452174 U:0 U1:55112931 r1bqkbr1/pppppppp/8/8/8/nn6/PPPPPPPP/RNBQKBNR b KQq - 0 0



Running with different thread counts gives differing results in the total number
of first-order non-pawn positions.

Threads Exec          Resolved  Pawn-Init-Pos Diff
8       2781s(46.4m)  23372033  299871125     +116381/1364076
7       3347s(55.8m)  23255652  298507049     + 24441/ 352446
6       3578s(59.6m)  23231211  298154603

I suspect this is due to undetected duplicates, so I'll need to write some
analysis programs to see if this is true. Oy.