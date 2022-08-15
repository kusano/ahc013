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
    vector<int> SP;
    // サーバーの各方向にケーブルがあるか
    vector<array<bool, 4>> SC;
    // ケーブル数
    int cn;
    // 位置 → サーバー
    // -1はサーバー無し
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

    // サーバーsから方向dに配線できるかどうかを返す
    bool can_connect(int s, int d)
    {
        if (cn>=K*100)
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
    bool can_connect2(int s, int d)
    {
        if (cn>=K*100)
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

    system_clock::time_point start = system_clock::now();

    my_exp_init();

    Room room(N, K, c);

    int score = room.score();
    
    int best_score = score;
    vector<vector<int>> best_connects;

    // 繋ぐために切断した配線
    vector<int> CC;

    double temp_inv;
    int iter;
    for (iter=0; ; iter++)
    {
        if (iter%0x1000==0)
        {
            system_clock::time_point now = system_clock::now();
            double time = chrono::duration_cast<chrono::nanoseconds>(now-start).count()*1e-9/TIME;
            if (time>1.0)
                break;
            double temp = 10*(1.0-time);
            temp_inv = 4./temp;
        }

        int s = 0;
        int d = 0;
        while (true)
        {
            s = xor64()%(K*100);
            d = xor64()%4;

            if (room.SC[s][d] ||
                room.can_connect2(s, d))
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

    cout<<0<<endl;
    cout<<best_connects.size()<<endl;
    for (auto c: best_connects)
        cout<<c[0]/N<<" "<<c[0]%N<<" "<<c[1]/N<<" "<<c[1]%N<<endl;    
}
