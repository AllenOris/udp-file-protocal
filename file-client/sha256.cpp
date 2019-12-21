#include "sha256.h"

sha256::sha256()
{

}

quint32 sha256::R(quint32 x, int p)
{
    return (x>>p);
}

quint32 sha256::S(quint32 x, int p)
{
    return (x>>p|x<<32-p);
}

quint32 sha256::z1(quint32 x)
{
    return S(x,6)^S(x,11)^S(x,25);
}

quint32 sha256::z0(quint32 x)
{
    return S(x,2)^S(x,13)^S(x,22);
}

quint32 sha256::ma(quint32 x, quint32 y, quint32 z)
{
    return (x&y)^(x&z)^(y&z);
}

quint32 sha256::ch(quint32 x, quint32 y, quint32 z)
{
    return (x&y)^(~x&z);
}

quint32 sha256::s1(quint32 x)
{
    return S(x,17)^S(x,19)^R(x,10);
}

quint32 sha256::s0(quint32 x)
{
    return S(x,7)^S(x,18)^R(x,3);
}

void sha256::has()
{
    quint32 x=0;
    int p=0;
    for(int i=0;i<512;++i){
        x<<=1;
        x+=Data[i];
        if((i+1)%32==0){
            w[p++]=x;
            x=0;
        }
    }
    for(int i=16; i<64;++i)
        w[i]=s1(w[i-2])+w[i-7]+s0(w[i-15])+w[i-16];
    quint32 a[8];
    for(int i=0;i<8;++i) a[i]=h[i];
    for(int i=0;i<64;++i){
        quint32 sum1=k[i]+w[i]+ch(a[4],a[5],a[6])+z1(a[4])+a[7];
        quint32 sum2=z0(a[0])+ma(a[0], a[1], a[2]);
        for(int j=7;j>=0;--j){
            if(j==4)a[j]=a[j-1]+sum1;
            else if(j==0)a[j]=sum1+sum2;
            else a[j]=a[j-1];
        }
    }
    quint32 ans=0;
    for(int i=0;i<8;++i){
        h[i]+=a[i];
    }
}


QString sha256::toHex(quint32  x)
{
    QString s;
    s.setNum(x, 16);
    while(s.size()<8)s="0"+s;
    return s;
}

QString sha256::hash(QByteArray arr)
{
    int sz=0;
    unsigned char c;
    for(int i=0;i<arr.size();++i){
        qDebug()<<i<<arr.size();
        c=quint32(arr[i]);
        sz+=8;
        QVector<bool>now;
        quint32 x=c;
        while(x){
            if(x&1)now.push_back(1);
            else now.push_back(0);
            x>>=1;
        }
        while(now.size()<8)now.push_back(x);
        reverse(now.begin(), now.end());
        for(auto x:now)Data.push_back(x);
        if(Data.size()==512){
            has();
            Data.clear();
        }
    }
    int cnt=Data.size();
    if(cnt>=448)cnt=448+512-cnt;
    else cnt=448-cnt;
    Data.push_back(1);
    --cnt;
    if(Data.size()==512){
        has();
        Data.clear();
    }
    while(cnt){
        Data.push_back(0), --cnt;
        if(Data.size()==512){
            has();
            Data.clear();
        }
    }
    QVector<bool> now;
    while(sz){
        if(sz&1)now.push_back(1);
        else now.push_back(0);
        sz>>=1;
    }
    while(now.size()<64)now.push_back(0);
    reverse(now.begin(), now.end());
    for(auto x: now)Data.push_back(x);
    has();
    Data.clear();
    QString res;
    for(int i=0;i<8;++i)res+=toHex(h[i]);
    return res;
}


