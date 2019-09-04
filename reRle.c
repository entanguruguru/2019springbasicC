#include <ctype.h>

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

    int x;
    char ch;
    int lowLen = 0;
    char strNum[16];

    while((ch = fgetc(fp)) != EOF){
        int attr;
        //isdigitで数字か文字か判定
        if(!(isdigit(ch))) strcpy(strNum, '1');
        while(isdigit(ch)){
            strcat(strNum, ch);
            ch = fgetc(pS);
        }

        switch (ch){
            case 'x':
                x = atoi(strNum);   //xもyもfprintfするがxのみがのちに必要なデータとなる
            case 'y':
                fprintf(pR, "%s\n",  strNum);
                break;

            case 'b':
                attr = 0;
                break;
            
            case 'o':
                attr = 1;
                break;

            case '$':
            case '!':
                snprintf(strNum, sizeof(strNum), "%d", x - lowLen);
                attr = 0;
                lowLen = 0;
                break;
            
            default:
                break;
        }

        for(int i = 0; i < atoi(strNum); i++){
            fprintf(pR, "%d", attr);
        }

        if(ch == '$' || ch == '!') fprintf(pR, "\n");
        else lowLen += atoi(strNum);
    }

    fclose(pS);
    fclose(pR);

    printf("変換が完了しました\n");
}