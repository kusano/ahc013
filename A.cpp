#include <iostream>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <array>
using namespace std;
using namespace std::chrono;

const double TIME = 2.8;

const int expN = 1024;
const double expX = 16;
int expT[expN];

void my_exp_init()
{
    for (int x=0; x<expN; x++)
    {
        double x2 = (double)-x/expN*expX;
        double e = exp(x2)*0x80000000+.5;
        if (e>=0x7fffffff)
            expT[x] = 0x7fffffff;
        else
            expT[x] = int(e);
    }
}

//  exp(t)*0x80000000;
int my_exp(double x)
{
    int x2 = int(x/-expX*expN+.5);
    if (x2<0)
        return expT[0];
    if (x2>=expN)
        return expT[expN-1];
    return expT[x2];
}

int xor64() {
    static uint64_t x = 88172645463345263ULL;
    x ^= x<<13;
    x ^= x>> 7;
    x ^= x<<17;
    return int(x&0x7fffffff);
}

struct Room
{
    int N = 0;
    int K = 0;
    // 向き
    // 0: →, 1: ↓, 2: ↑, 3: ←
    vector<int> D;
    // サーバーの位置
    // 種類順に格納するので、/100で種類になる
    vector<int> SPorg;
    vector<int> SP;
    // サーバーの各方向にケーブルがあるか
    vector<array<bool, 4>> SC;
    // 移動したコンピューター数
    int mn;
    // ケーブル数
    int cn;
    // 位置 → サーバー
    // -1はサーバー無し
    vector<int> RSorg;
    vector<int> RS;
    // 位置 → ケーブル
    // サーバー*4+向き
    // どちら側のサーバーでも良い
    // -1はケーブル無し
    vector<int> RC;
    //  汎用一時配列
    vector<int> T;

    Room(int N, int K, vector<string> c)
        : N(N)
        , K(K)
        , D(4)
        , SP(K*100)
        , SC(K*100)
        , mn(0)
        , cn(0)
        , RS(N*N, -1)
        , RC(N*N, -1)
        , T(K*100)
    {
        D[0] = 1;
        D[1] = N;
        D[2] = -1;
        D[3] = -N;

        vector<int> KI(K);
        for (int y=0; y<N; y++)
            for (int x=0; x<N; x++)
            {
                int k = c[y][x]-'0';
                if (k>0)
                {
                    k--;
                    SP[k*100+KI[k]] = y*N+x;
                    RS[y*N+x] = k*100+KI[k];
                    KI[k]++;
                }
            }

        SPorg = SP;
        RSorg = RS;
    }

    int score()
    {
        for (int &t: T)
            t = 0;

        int sc = 0;
        for (int i=0; i<K*100; i++)
            if (T[i]==0)
            {
                int KN[5] = {};
                score_f(i, KN, -1);

                int s = 0;
                for (int n: KN)
                    s += n;

                for (int n: KN)
                    sc += n*(n-1) - n*(s-n);
            }
        return sc/2;
    }

    void score_f(int s, int KN[5], int pd)
    {
        if (T[s]!=0)
            return;
        T[s] = 1;
        KN[s/100]++;

        for (int d=0; d<4; d++)
            if (d!=(pd^2) && SC[s][d])
            {
                int p = SP[s] + D[d];
                while (RS[p]==-1)
                    p += D[d];
                score_f(RS[p], KN, d);
            }
    }

    // 同じ種類のコンピューターが接続可能な配線数を返す
    int score2()
    {
        int s = 0;
        for (int y=0; y<N; y++)
        {
            int pk = -1;
            for (int x=0; x<N; x++)
                if (RS[y*N+x]!=-1)
                {
                    int k = RS[y*N+x]/100;
                    if (k==pk)
                        s += k==0?5:1;
                    pk = k;
                }
        }
        for (int x=0; x<N; x++)
        {
            int pk = -1;
            for (int y=0; y<N; y++)
                if (RS[y*N+x]!=-1)
                {
                    int k = RS[y*N+x]/100;
                    if (k==pk)
                        s += k==0?5:1;
                    pk = k;
                }
        }
        return s;
    }

    // サーバーsを初期位置から向きdに移動できるかを返す
    // d=-1で初期位置に戻す
    bool can_move(int s, int d, int Xmax)
    {
        if (d==-1)
            return true;
        if (mn>=Xmax)
            return false;

        int p = SPorg[s];
        if (d==0 && p%N==N-1 ||
            d==1 && p/N==N-1 ||
            d==2 && p%N==0 ||
            d==3 && p/N==0)
            return false;

        int p2 = p + (d==-1 ? 0 : D[d]);
        if (RS[p2]!=-1 || RSorg[p2]!=-1)
            return false;

        return true;
    }

    // サーバーを移動する
    void move(int s, int d)
    {
        if (SP[s]!=SPorg[s])
            mn--;

        RS[SP[s]] = -1;
        SP[s] = SPorg[s] + (d==-1 ? 0 : D[d]);
        RS[SP[s]] = s;

        if (SP[s]!=SPorg[s])
            mn++;
    }

    void get_moves(vector<vector<int>> *moves)
    {
        moves->clear();
        for (int s=0; s<100*K; s++)
            if (SP[s]!=SPorg[s])
                moves->push_back({SPorg[s], SP[s]});
    }

    // サーバーsから方向dに配線できるかどうかを返す
    bool can_connect(int s, int d, int Ymax)
    {
        if (cn>=Ymax)
            return false;

        int p = SP[s];
        while (true)
        {
            if (d==0 && p%N==N-1 ||
                d==1 && p/N==N-1 ||
                d==2 && p%N==0 ||
                d==3 && p/N==0)
                return false;
            p += D[d];
            if (RS[p]!=-1)
                return true;
            if (RC[p]!=-1)
                return false;
        }
    }

    // 既存の配線は無視してサーバーsから方向dに配線できるかどうかを返す
    bool can_connect2(int s, int d, int Ymax)
    {
        if (cn>=Ymax)
            return false;

        int p = SP[s];
        while (true)
        {
            if (d==0 && p%N==N-1 ||
                d==1 && p/N==N-1 ||
                d==2 && p%N==0 ||
                d==3 && p/N==0)
                return false;
            p += D[d];
            if (RS[p]!=-1)
                return true;
        }
    }

    void connect(int s, int d)
    {
        cn++;

        SC[s][d] = true;

        int p = SP[s];
        while (true)
        {
            p += D[d];
            if (RS[p]!=-1)
                break;
            RC[p] = s*4+d;
        }

        SC[RS[p]][d^2] = true;
    }

    void cut(int s, int d)
    {
        cn--;

        SC[s][d] = false;

        int p = SP[s];
        while (true)
        {
            p += D[d];
            if (RS[p]!=-1)
                break;
            RC[p] = -1;
        }

        SC[RS[p]][d^2] = false;
    }

    void get_connects(vector<vector<int>> *connects)
    {
        connects->resize(0);
        for (int s=0; s<K*100; s++)
            for (int d=0; d<2; d++)
                if (SC[s][d])
                {
                    int p = SP[s];
                    while (true)
                    {
                        p += D[d];
                        if (RS[p]!=-1)
                            break;
                    }

                    connects->push_back(vector<int>{SP[s], p});
                }
    }
};

int main()
{
    int N, K;
    cin>>N>>K;
    vector<string> c(N);
    for (string &t: c)
        cin>>t;

    cerr<<"N: "<<N<<endl;
    cerr<<"K: "<<K<<endl;
    cerr<<"100*K/N/N: "<<100.*K/N/N<<endl;

    system_clock::time_point start = system_clock::now();

    my_exp_init();

    Room room(N, K, c);

    // 移動
    int Xmax = 0;
    switch (K)
    {
    case 2: Xmax = N-15; break;
    case 3: Xmax = N+30; break;
    case 4: Xmax = N+80; break;
    case 5: Xmax = N+150; break;
    default:
        return 1;
    }

    int score2 = room.score2();
    int best_score2 = score2;
    vector<vector<int>> best_moves;

    double temp_inv;
    int iter;
    if (Xmax>0)
        for (iter=0; ; iter++)
        {
            if (iter%0x100==0)
            {
                system_clock::time_point now = system_clock::now();
                double time = chrono::duration_cast<chrono::nanoseconds>(now-start).count()*1e-9/(TIME/10);
                if (time>1.0)
                    break;
                double temp = 10*(1.0-time);
                temp_inv = 1./temp;
            }

            int s = 0;
            int d = 0;
            while (true)
            {
                s = xor64()%(K*100);
                if (room.SP[s]==room.SPorg[s])
                    d = xor64()%4;
                else
                    d = -1;
                if (room.can_move(s, d, Xmax))
                    break;
            }

            int diff = room.SP[s]-room.SPorg[s];
            int dold = 0;
            if (diff==0) dold = -1;
            else if (diff==1) dold = 0;
            else if (diff==N) dold = 1;
            else if (diff==-1) dold = 2;
            else if (diff==-N) dold = 3;
            else
                return -1;

            room.move(s, d);

            int score22 = room.score2();

            if (score22>score2 ||
                //exp((score2-score)*temp_inv)*0x80000000>xor64())
                my_exp((score22-score2)*temp_inv)>xor64())
            {
                score2 = score22;

                if (score2>best_score2)
                {
                    best_score2 = score2;
                    room.get_moves(&best_moves);
                }
            }
            else
                room.move(s, dold);
        }

    room.SP = room.SPorg;
    room.RS = room.RSorg;
    for (auto m: best_moves)
    {
        int s = room.RSorg[m[0]];
        room.SP[s] = m[1];
        room.RS[m[0]] = -1;
        room.RS[m[1]] = s;
    }

    cerr<<"Iteration: "<<iter<<endl;
    cerr<<"Score: "<<best_score2<<endl;
    cerr<<"X/Xmax: "<<best_moves.size()<<"/"<<Xmax<<endl;

    // 配線
    int Ymax = 100*K - (int)best_moves.size();

    int score = room.score();
    
    int best_score = score;
    vector<vector<int>> best_connects;

    // 繋ぐために切断した配線
    vector<int> CC;

    for (iter=0; ; iter++)
    {
        if (iter%0x1000==0)
        {
            system_clock::time_point now = system_clock::now();
            double time = chrono::duration_cast<chrono::nanoseconds>(now-start).count()*1e-9/TIME;
            if (time>1.0)
                break;
            double temp = 2.5*(1.0-time);
            temp_inv = 1./temp;
        }

        int s = 0;
        int d = 0;
        while (true)
        {
            s = xor64()%(K*100);
            d = xor64()%4;

            if (room.SC[s][d] ||
                room.can_connect2(s, d, Ymax))
                break;
        }

        if (room.SC[s][d])
            room.cut(s, d);
        else
        {
            CC.clear();
            int p = room.SP[s];
            while (true)
            {
                p += room.D[d];
                if (room.RS[p]!=-1)
                    break;
                if (room.RC[p]!=-1)
                {
                    CC.push_back(room.RC[p]);
                    room.cut(room.RC[p]/4, room.RC[p]%4);
                }
            }

            room.connect(s, d);
        }

        int score2 = room.score();

        if (score2>score ||
            //exp((score2-score)*temp_inv)*0x80000000>xor64())
            my_exp(-sqrt(score-score2)*temp_inv)>xor64())
        {
            score = score2;

            if (score>best_score)
            {
                best_score = score;
                room.get_connects(&best_connects);
            }
        }
        else
        {
            if (!room.SC[s][d])
                room.connect(s, d);
            else
            {
                room.cut(s, d);

                for (int c: CC)
                    room.connect(c/4, c%4);
            }
        }
    }

    cerr<<"Iteration: "<<iter<<endl;
    cerr<<"Score: "<<best_score<<endl;
    cerr<<"Y/Ymax: "<<best_connects.size()<<"/"<<Ymax<<endl;
    cerr<<endl;

    cout<<best_moves.size()<<endl;
    for (auto m: best_moves)
        cout<<m[0]/N<<" "<<m[0]%N<<" "<<m[1]/N<<" "<<m[1]%N<<endl;
    cout<<best_connects.size()<<endl;
    for (auto c: best_connects)
        cout<<c[0]/N<<" "<<c[0]%N<<" "<<c[1]/N<<" "<<c[1]%N<<endl;
}
