/*
    Game of life
    ファイル読み込み機能を持ったライフゲーム
    
    操作方法
    1.はじめに使う際はソースコード冒頭にある定数を調整して動作環境に合わせる

    2.ライフゲームの初期配置はランダムに配置するか外部からパターンファイルを読み込むことができる、
      ターミナルに表示される言葉に従い"r""c""s"のいずれかを入力する、それ以外が入力されるとプログラムは終了する

    3."r"を選択した時
    　numRandomSell 定数にあらかじめ設定された数値分だけセルがランダムに初期配置として代入される
    　 "c"を選択した時
    　ファイル名を入力すると（作成手順は後述するが）読み込み用のファイル(.rleファイルをアレンジしたもの）をプログラムが読み取れる状態に解析・変換してくれる
    　変換されたファイルは別途生成されるので内容を確認したらターミナルにその名前を入力することで初期配置を設定することができる
    　 "s"を選択した時
    　"c"を選択した時の処理からファイル変換の工程がなくなったものである、"c"を使い変換されたファイル名を入力するとそのファイルに従って初期配置が設定される

    4.　3のいずれかの処理が終了すると2つのウィンドウが表示される
        初期状態を表示した状態で停止しているので、状態変化を開始したいときはMainMonitorをクリックしていづれかのキーボードのボタンを押せば良い

    5. プログラムの終了は２つのモニターのいづれかの左上にあるxマークをクリックする

    ＊読み込み用ファイルの作成手順(別途説明用の資料を添付する)
        1. パターンファイルを
                http://www.conwaylife.com/wiki/Category:Patterns
                や
                https://copy.sh/life/examples/
           から .rle (ランレングス圧縮された)ファイルとして入手する

        注意事項：上記サイトで手に入ったファイルの中にはデータの圧縮の仕方に違いがあるものがあるのでうまく動かないこともある

           >.rleファイルについてはhttp://www.conwaylife.com/wiki/RLE を参照してもらいたい

        2. .rleファイルの内容を確認する
        3.#に続く部分の情報は無視をして
        　 x = ~ , y = ~ とパターンファイルの高さと幅が書いてあるので読み込み用ファイルの冒頭に
                            ~x~y (~の部分は読み取った整数値)
          と書いたうしろに 整数、b、o、$、! で表されている部分をコピーして作成完了

        変換後のファイルは先頭に横幅x（一行分のデータ数）の値と高さy（一列分のデータ数）の値、
        その後に　0と1の合計個数がx（またはy）をちょうど満たすようにスペースで区切られた並んだテキストファイルである

    （以上の補足資料と読み込み用のサンプルデータを同封しておく）


    2019.5.29- Nishimoto Kazuki
    2019.6.26 コードの一部を関数化
    2019.6.28 ファイルの読み込み機能実装
    2019.7.10 rleファイル（自作）の読み込み、変換機能
    2019.7.11　ダブルバッファリング
    他　適宜微調整
*/
/*
    状態変化のための処理
    arrayXを捜索用の配列,arrayYを捜索結果のメモかつ描写元の配列として設定
    ① 初期状態をX,Yへ入力
    ② Yの様子を描写
    ⓷ Y→Xへコピー
    ④ Xを調べてYへ結果を代入（上書き保存）
    ⑤ Yを描写
    ⑥ ③ー⑤をループ
    サブの処理
    SIZE*SIZEの二次元行列の外周部を全て定数０にして捜索は(SIZE-1)*(SIZE-1)の範囲
    そのためには代入する際調整が必要
    （配列空間の変更と描写空間の調整が必要）
*/

#include <stdio.h>
#include <handy.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

/*命名規則について　定数      > 全て大文字 
                 関数、変数 > 先頭小文字
*/

//使用する画面に応じて変更する、パターンファイルがおさまるサイズにすること
#define SIZE 250+1
#define gridSpace 2.6
//初期状態をランダムで与える時のセルの数
#define numRandomSell 10000
//描写速度の調整部分、処理速度がボトルネックとなり速度に上限あり
#define timeSleep 0.02

void randomStart(int arrayX[SIZE][SIZE], int arrayY[SIZE][SIZE]);
void RLEconverter();
void readFile(int arrayY[SIZE][SIZE]);
void setArrays(int arrayX[SIZE][SIZE], int arrayY[SIZE][SIZE]);
void setWindows(int *mainW, doubleLayer *layerD, int *subW, int *statusLayer);
void arrayYtoX(int arrayX[SIZE][SIZE], int arrayY[SIZE][SIZE]);
void drawSell(int *sellLayer, doubleLayer *layerD, int arrayY[SIZE][SIZE]);
void mooreN(int arrayX[SIZE][SIZE], int arrayY[SIZE][SIZE]);
void countSell(int arrayX[SIZE][SIZE], int arrayY[SIZE][SIZE], int count, int statusLayer);

int main(){
    int arrayX[SIZE][SIZE] = {};
    int arrayY[SIZE][SIZE] = {};

    int mainW, sellLayer;
    doubleLayer layerD;
    int  subW, statusLayer;

    int  count;
    char firstSelect;

    printf("初期状態の入力方法を選択してください\n"
           "(ランダム-> r / ファイル変換-> c / ファイル読み込み-> s)\n");
    scanf("%s", &firstSelect);

/*
　switchはタコ足、評価対象一つを握りしめて定数値と比較する。
　if,else は条件式に対して一つづつ順番に比較する。  if,elseで同じ関数を連続して呼び出す場合その返り値が全て同じとは限らないので危険である。
　構造が違うことを意識してコードに織り込むことで他人が見た時によりわかりやすくミスの起きにくいコードになる。
*/
    switch (firstSelect){
        case 'r':
            //初期状態をarrayYに入力する
            randomStart(arrayX, arrayY);
            break;

        case 'c':
            //rleファイルを変換する
            RLEconverter();
            printf("変換したファイルが処理できる内容であるか確認、修正を行ってください\n\n");
            //変換後のファイル内容を取り込む
            readFile(arrayY);
            break;

        case  's':
            //変換後のファイル内容を取り込む
            readFile(arrayY);
            break;

        default:
            //指定された文字以外が入力された時プログラムを終了する
            printf("指定外の文字が入力されました、中止します\n");
            return 0;
    }

    //arrayXとarrayYの最外周部に0を配置
    setArrays(arrayX, arrayY);

    //画面とレイヤーを準備する
    setWindows(&mainW, &layerD, &subW, &statusLayer);

    //配列の最外周部を除いて描写することに注意
    for(count = 0; ; count++){
        //arrayYをarrayXにコピー
        arrayYtoX(arrayX, arrayY);

        //(arrayYを参照して)セルの描写
        drawSell(&sellLayer, &layerD, arrayY);
        HgSleep(timeSleep);

        //subWindow(の上のレイヤー）に現状を表示
        countSell(arrayX, arrayY, count, statusLayer);

        
        //arrayXを調べてその結果をarrayYに上書きする
        mooreN(arrayX, arrayY);

        if(count == 1) HgGetChar();
    }

    HgGetChar();
    HgClose();
    HgWClose(subW);
    return 0;
}

void randomStart(int arrayX[SIZE][SIZE], int arrayY[SIZE][SIZE]){
    int row, colm;
    srand(time(NULL));
    int numSell = 0;
    do {
        colm = rand() % SIZE;
        row  = rand() % SIZE;
        if (arrayY[row][colm] == 0){
            arrayY[row][colm] = 1;
            numSell++;
        }
    } while (numSell < numRandomSell);
}


//改編後

void RLEconverter(){
    FILE *pS, *pR;
    char summaryName[100];
    char resultName[100];

    printf("開きたいrleファイル名を入力してください\n");
    scanf("%s", summaryName);
    printf("結果を書き出したいファイルの名前を入力してください\n");
    scanf("%s", resultName);

    pS = fopen(summaryName, "r");
    pR = fopen(resultName, "w");

    //ファイルエラーチェック
    if(pS == NULL){
        printf("ファイルを開けませんでした\n");
    }
    if(pR == NULL){
        printf("ファイルを開けませんでした\n");
    }

    int x;          //欠損データ補完のための変数
    int lowLen = 0; //欠損データ補完のためのカウンタ変数
    char ch;        //ファイルから読み取った現在の文字

    while((ch = fgetc(pS)) != EOF){
        char strNum[16] = "";
        int attr;   //変換後に出力するデータを保管する変数

        //isdigitで数字か文字か判定
        if(!(isdigit(ch))) strcpy(strNum, "1");
        while(isdigit(ch)){
            snprintf(strNum, sizeof(strNum), "%s%c", strNum, ch);
            ch = fgetc(pS);
        }
        switch (ch){
            case 'x':
                x = atoi(strNum);   //xもyもfprintfするが欠陥データの処理はx方向に関して必要なのでxのみ文字数を保存
            case 'y':
                fprintf(pR, "%s\n",  strNum);
                continue;

            case 'b':       //bは空白つまり0に変換
                attr = 0;
                break;
            
            case 'o':       //oは生存つまり1に変換
                attr = 1;
                break;

            case '$':       //1行分の読み取りが完了した時点で欠損データの評価と補完を行う
            case '!':
                snprintf(strNum, sizeof(strNum), "%d", x - lowLen);
                attr = 0;
                lowLen = 0; //欠損データの評価が終了したのでカウンタ変数を初期化
                break;
            
            default:
                continue;
        }

        for(int i = 0; i < atoi(strNum); i++){
            fprintf(pR, "%d ", attr);
        }

        if(ch == '$' || ch == '!' ) fprintf(pR, "\n");      //1行分のデータ変換が終了した時点で改行
        else lowLen += atoi(strNum);                        //1行の終わりまでは欠損データ対策のためにカウンタ変数で現在の文字数を覚えておく
    }

    fclose(pS);
    fclose(pR);

    printf("変換が完了しました\n");
}

void readFile(int arrayY[SIZE][SIZE]){
    FILE *fp;
    char fileName[100];
    int row, colm;
    int y, x;
    int positionY, positionX;

    printf("変換済みのファイル名を入力してください\n");
    scanf("%s", fileName);

    fp = fopen(fileName, "r");

    if(fp == NULL){
        printf("ファイルを開けませんでした\n");
    }

    fscanf(fp, "%d %d", &x, &y);

    //初期配置の位置を中心にする
    positionY = (SIZE - y)/2;
    positionX = (SIZE - x)/2;
    
    for(row = 1; row <= y; row++){
        for(colm = 1; colm <= x; colm++){
            fscanf(fp, "%d", &arrayY[row+positionY][colm+positionX]);
        }
    }

    fclose(fp);
}

void setArrays(int arrayX[SIZE][SIZE], int arrayY[SIZE][SIZE]){
    int row, colm;
    for(row = 0; row <= SIZE; row++){
        for(colm = 0;colm <= SIZE; colm++){
            if(row == 0 || row == SIZE || colm == 0 || colm == SIZE){
                arrayX[row][colm] = 0;
                arrayY[row][colm] = 0;
            }
        }
    }
}

void setWindows(int *mainW, doubleLayer *layerD, int *subW, int *statusLayer){
    //受け取る値はアドレスであることに注意
    //メインウィンドウ準備
    *mainW = HgOpen(gridSpace*(SIZE-1), gridSpace*(SIZE-1));
    HgWSetTitle(*mainW, "MainMonitor");
    //メインウィンドウを黒く塗りつぶす
    HgSetFillColor(HG_BLACK);
    HgBoxFill(0, 0, gridSpace*(SIZE-1), gridSpace*(SIZE-1),1);

    //メインウィンドウのセル描写用のレイヤー準備
    *layerD = HgWAddDoubleLayer(*mainW);

    //サブウィンドウ準備
    *subW  = HgWOpen(0, 0, 300, 150);
    HgWSetTitle(*subW, "SubMonitor");

    HgWSetFont(*subW, HG_UTF8_CODE, 13);
    HgWText(*subW,  5, 30, "開始:MainMonitorをクリックしてキーボードを押す\n"
                             "終了:ウィンドウのxボタンをクリック");

    //セルのステータスの表示用レイヤーの準備
    *statusLayer = HgWAddLayer(*subW);
}

void arrayYtoX(int arrayX[SIZE][SIZE], int arrayY[SIZE][SIZE]){
    int row, colm;
    for(row = 1; row <= (SIZE-1); row++){
        for(colm = 1; colm <= (SIZE-1); colm++){
            arrayX[row][colm] = arrayY[row][colm];
        }
    }
}

//捜索範囲調整のため 1 <= (row,colm) <= (SIZE-1)となる

//全てのセルを捜索し生きていれば(１ならば)レイヤーに描写する
void drawSell(int *sellLayer, doubleLayer *layerD, int arrayY[SIZE][SIZE]){
    int row, colm;
    *sellLayer = HgLSwitch(&*layerD);
    HgLClear(*sellLayer);
    HgWSetFillColor(*sellLayer, HG_GREEN);
    for(row = 1; row <= (SIZE-1); row++){
        for(colm = 1; colm <= (SIZE-1); colm++){
            if(arrayY[row][colm] == 1){
                //条件と描写は調整が必要なことに注意
                HgWBoxFill(*sellLayer, gridSpace*(colm-1), gridSpace*(SIZE-1-row), gridSpace, gridSpace, 0);
            }
        }
    }
    return;
}

void mooreN(int arrayX[SIZE][SIZE], int arrayY[SIZE][SIZE]){
    int row, colm;
    //row,colmを変えてムーア近傍の中心座標を移動させる
    int sum8;   //外部総和
    for(row = 1;row <= (SIZE-1); row++){
        for(colm = 1; colm <= (SIZE-1); colm++){
            sum8 = arrayX[row-1][colm-1]
                 + arrayX[row-1][colm  ]
                 + arrayX[row-1][colm+1]
                 + arrayX[row  ][colm-1]
                 + arrayX[row  ][colm+1]
                 + arrayX[row+1][colm-1]
                 + arrayX[row+1][colm  ]
                 + arrayX[row+1][colm+1];

            //前状態のarrayXから得られた外部総和をもとに次状態であるarrayYを上書き
            switch (sum8){
                //"過疎"
                case 0:
                    arrayY[row][colm] = 0;
                    break;
                case 1:
                    arrayY[row][colm] = 0;
                    break;
                //"維持"なので何もしない
                case 2:
                    arrayY[row][colm] = arrayX[row][colm];
                    break;
                //"誕生"
                case 3:
                    arrayY[row][colm] = 1;
                    break;
                //"過密"
                case 4:
                    arrayY[row][colm] = 0;
                    break;
                case 5:
                    arrayY[row][colm] = 0;
                    break;
                case 6:
                    arrayY[row][colm] = 0;
                    break;
                case 7:
                    arrayY[row][colm] = 0;
                    break;
                case 8:
                    arrayY[row][colm] = 0;
                    break;
                default:
                    break;
            }
        }
    }
    return;
}

void countSell(int arrayX[SIZE][SIZE], int arrayY[SIZE][SIZE], int count, int statusLayer){
    int row, colm;
    //arrayYの各セルを調べて生存セル数を表すcountSの値を変える
    int countS;
    for(row = 1; row <= (SIZE-1); row++){
        for(colm = 1; colm <= (SIZE-1); colm++){
            if(arrayY[row][colm] == 1){
                countS++;
            }
        }
    }
    HgLClear(statusLayer);
    HgWSetFont(statusLayer, HG_UTF8_CODE, 18);
    HgWText(statusLayer, 140, 100, "生存セル数:%d", countS);
    HgWText(statusLayer, 40, 100, "第%d世代", count);

    return;
}