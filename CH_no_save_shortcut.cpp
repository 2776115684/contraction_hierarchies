#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define pb push_back
#define mp make_pair
#define fs first
#define sc second

using namespace std;

typedef long long ll;                                                     // ������
typedef pair<int, int> Node;                                                // ���: (�����, ����)
typedef vector<vector<Node>> Graph;                                       // ͼ: �ڽӱ�
typedef priority_queue<Node, vector<Node>, greater<Node>> DijkstraQueue;  // ��С��

typedef struct cars {        //�洢�����������Ϣ����ʼ������������ֵ������������...��
    double remaining_power;   //ʣ��ٷֱȵ���%
    double min_power;       //����ٷֱȵ�����ֵ%
    double max_distance;    //��������������km
    double battery_capacity;//��������������kW��h

    //����������x��y�ĺ���,x��y��ԭͼ����
    double power_consume(int distance) {
        return distance / max_distance * 100.0;
    }

    double distanceCanRun() {
        return (remaining_power - min_power) / 100 * max_distance;
    }
}bev;

const int INF = 1e9 + 10;  // �����

// ifstream graph("NY_graph.txt");  // ��ȡͼ
// ifstream queries("Queries.txt"); // ��ȡ��ѯ

ifstream queries("query2.txt");

// ����һ��ofstream���󣬲���һ����Ϊ"output.txt"���ļ�
std::ofstream outFile("output.txt");

class CH {
private:

    int n, m, s, t, order;                       // n: �����, m: ����, s: ���, t: �յ�, order: ���˳��
    vector<Graph> G, G_CH;                       // G[0] ��ʾ�����(u Ϊ�ߵ����), G[1] ��ʾ�����(u Ϊ�ߵ��յ�); G_CH �ǰ����ݾ��ĵ���ͼ
    vector<vector<int>> dist;                     // ���ڴ洢��ÿ����㵽��������������̾���
    vector<DijkstraQueue> pq;                    // ���ڴ洢������Ľ��; ���������ȶ��зֱ��Ӧ�����pq[0]�ͷ����pq[1]
    DijkstraQueue imp_pq;                        // ��Ҫ�����ȶ���
    vector<bool> contracted;                     // �жϽ���Ƿ��Ѿ�����
    vector<int> imp;                             // �洢�����Ҫ��, ����ȷ���������˳��
    vector<int> level;                           // ��¼ÿ�����Ĳ㼶
    vector<int> contr_neighbours;                // ��¼ÿ�����������ھ���
    unordered_map<string, vector<int>> shortcut;  // ��¼�ݾ���ԭ·��
    unordered_map<string, int> shortcut_weight;    // ��¼�ݾ���Ȩ��
    vector<vector<int>> fa;                       // ��¼�ڵ�ĸ��ڵ�, fa[0]:����߸��ڵ�, fa[1]:����߸��ڵ�
    vector<int> stations;                         // ��¼���վ�Ľڵ�
    bev car;                                      // ��¼��������Ϣ
    friend class CH_ChargeStation;                // ������Ԫ��

    // ��ȡͼ(��������ͼ; ���Ҫ��������ͼ����Ҫ��ÿ���߶�������)
    void read(string input) {
        ifstream graph(input);
        graph >> n >> m;
        G.assign(2, Graph(n + 1, vector<Node>()));  // ��ʼ��ͼ
        int u, v, w;                                // u: �ߵ����, v: �ߵ��յ�, w: �ߵ�Ȩ��
        for (int i = 0; i < m; ++i) {
            graph >> u >> v >> w;
            //u++, v++;    // ����Ŵ� 1 ��ʼ
            if (u == v)  // �ų��Ի�
                continue;
            Connect(G[0][u], v, w);  // �����, ��ʾ�ӽ�� u �����������
            Connect(G[1][v], u, w);  // �����, ��ʾָ���� v �ķ����
            Connect(G[0][v], u, w);  // ����ͼ��Ҫ��ÿ���߶�������
            Connect(G[1][u], v, w);  // ����ͼ��Ҫ��ÿ���߶�������
        }
        int size;
        graph >> size;
        stations.assign(size, false);
        for (int i = 0; i < size; i++) {
            int x;
            graph >> x;
            stations[i] = x;
        }
    }

    // Ԥ����
    void preprocess() {
        SetOrder();   // ���ý��˳��
        BuildG_CH();  // ���� CH ͼ
    }

    // ���ӽڵ�
    void Connect(vector<Node>& E, int v, int w) {
        // ����Ƿ����ر�
        for (auto& p : E) {
            if (p.fs == v) {
                p.sc = min(p.sc, w);  // ������ر�, ȡ��Сֵ
                return;               // ����, ������±�
            }
        }
        E.pb(mp(v, w));  // ���û���ر�, ����±�
    }

    // ��ӽݾ�ԭ·��
    void addShortcut(int v, int mid, int u, int distance) {
        // ���ɼ�
        string str_v = to_string(v);
        string str_mid = to_string(mid);
        string str_u = to_string(u);
        string v_mid = str_v + "?" + str_mid;
        string mid_u = str_mid + "?" + str_u;
        string v_u = str_v + "?" + str_u;
        //�жϽݾ��Ƿ��Ѵ���/����
        if (shortcut_weight.find(v_u) != shortcut_weight.end()) {
            if (shortcut_weight[v_u] <= distance) return;
            else shortcut[v_u].clear();
        }
        else {
            shortcut_weight[v_u] = distance;
        }
        // �ж�v-mid-u�Ƿ�����ݾ�
        vector<int> vec;
        if (shortcut.find(v_mid) != shortcut.end()) {
            vec.insert(vec.end(), shortcut[v_mid].begin(), shortcut[v_mid].end());
        }
        else {
            vec.insert(vec.end(), { v, mid });
        }
        if (shortcut.find(mid_u) != shortcut.end()) {
            vec.insert(vec.end(), shortcut[mid_u].begin() + 1, shortcut[mid_u].end());
        }
        else {
            vec.insert(vec.end(), { u });
        }
        shortcut[v_u] = vec;
        return;
    }

    // ���ýڵ�˳��
    void SetOrder() {
        contracted.assign(n + 1, 0);        // ��ʼ���ڵ�����״̬(��δ����)
        imp.assign(n + 1, 0);               // ��ʼ���ڵ���Ҫ��
        level.assign(n + 1, 0);             // ��ʼÿ�����Ĳ㼶
        contr_neighbours.assign(n + 1, 0);  // ��ʼ��ÿ�����������ھ���

        for (int i = 1; i <= n; ++i)
            imp_pq.push(mp(-n, i));  // ��ʼ����Ҫ�����ȶ���(����Ϊ -n ��֤�����ȴ���)
        int curr_node, new_imp;
        order = 1;
        while (!imp_pq.empty()) {
            curr_node = imp_pq.top().sc;
            imp_pq.pop();
            new_imp = GetImportance(curr_node);  // ��ȡ�ڵ���Ҫ��
            // �����ǰ�ڵ����Ҫ���뵱ǰ��С��Ҫ��֮��С�ڵ���10, ��������ǰ�ڵ�
            // ��ʹ��ǰ�ڵ㱻��������һ���ڵ����������Ҳ�����ܵ�̫��Ӱ��; ������Ч���ƽڵ�������Ƶ��
            if (imp_pq.empty() || new_imp - imp_pq.top().fs <= 10) {
                imp[curr_node] = order++;  // ���ýڵ�˳��
                contracted[curr_node] = 1;
                ContractNode(curr_node);
            }
            else {
                imp_pq.push(mp(new_imp, curr_node));  // ����, ����ǰ�ڵ����¼������ȶ���
            }
        }
    }

    // ��ȡ�ڵ���Ҫ��
    int GetImportance(int x) {
        int u, v, shortcuts = 0, in_out = 0;  // shortcuts: ��Ч���·����, in_out: ԭʼ����
        for (int i = 0; i < 2; ++i) {
            for (auto& p : G[i][x]) {
                if (!contracted[p.fs])  // ����ڽӽ��δ������, ������ԭʼ����
                    ++in_out;           // ����ԭʼ����
            }
        }
        for (auto& p1 : G[1][x]) {
            for (auto& p2 : G[0][x]) {
                u = p1.fs;
                v = p2.fs;  // u, v �ֱ��ʾ x ����ߺͳ��ߵ��ڽӽ��(�ݾ�u-x-v)
                // ��� u �� v ��δ������, ��������Ч���·����
                if (!contracted[u] && !contracted[v])
                    ++shortcuts;
            }
        }
        int edge_diff = shortcuts - in_out;  // ��Ե���� = ��ӵĿ�ݷ�ʽ���� - �Ƴ���ԭʼ����

        // ���ؽڵ���Ҫ��; ��Ҫ�� = ��Ե���� + 2 * �����ھ��� + �㼶
        // *2 �Ŵ������ھ������Խڵ���Ҫ��
        return edge_diff + 2 * contr_neighbours[x] + level[x];
    }

    // �����ڵ�
    void ContractNode(int x) {
        int u;
        int w, mx = GetMaxEdge(x);
        set<pair<int, int>> out_edges;
        for (auto& p : G[0][x]) {
            if (!contracted[p.fs])
                out_edges.insert(mp(p.fs, p.sc));
        }
        for (auto& p : G[1][x]) {
            u = p.fs;
            if (contracted[u])
                continue;
            w = p.sc;
            Check_Witness(u, x, w, mx, out_edges, 0);  // ����֤+ѹ��
        }
        for (int i = 0; i < 2; ++i) {
            for (auto& p : G[i][x]) {
                ++contr_neighbours[p.fs];
                level[p.fs] = max(level[p.fs], level[x] + 1);  // �����ھӵĲ㼶
            }
        }
    }

    // ��ȡ����Ȩ (��Ȩ��Ľ����Ҫ����)
    int GetMaxEdge(int x) {
        int ret = 0;
        for (auto& p1 : G[1][x]) {
            for (auto& p2 : G[0][x]) {
                if (p1.fs != p2.fs && !contracted[p1.fs] && !contracted[p2.fs])
                    ret = max(ret, p1.sc + p2.sc);
            }
        }
        return ret;
    }

    // ����֤ --        u-x-out_edges
    int Check_Witness(int u, int x, int w, int mx, set<pair<int, int>>& out_edges, bool type) {
        int a, b;                // a: ��ǰ���, b: �ڽӽ��
        int curr_dist, new_dist;  // curr_dist: ��ǰ����, new_dist: �¾���
        DijkstraQueue D_pq;
        unordered_map<int, int> D_dist;
        D_pq.push(mp(0, u));
        D_dist[u] = 0;
        int iter = 250 * (n - order) / n;  // ��Ϊ�趨һ��������������Ϊ�ں��ڽ��������ɱ��ܸ�
        // ��u��������������ܱ߷��䣬���u
        //  ��ʼ��·���ͣ�������D_dist[b]��b��ʾ�յ㡣
        while (!D_pq.empty() && iter--) {
            curr_dist = D_pq.top().fs;
            a = D_pq.top().sc;
            D_pq.pop();
            if (curr_dist > D_dist[a])
                continue;
            for (auto& p : G[0][a]) {
                new_dist = p.sc + curr_dist;
                b = p.fs;
                if (b != x && !contracted[b]) {
                    if (D_dist.find(b) == D_dist.end() || D_dist[b] > new_dist) {
                        // D_dist[b] < mx��������Ч���ݣ���D_dist[b] >= mx, a-x-bһ������ѹ��
                        if (D_dist[b] < mx)
                            D_dist[b] = new_dist, D_pq.push(mp(new_dist, b));
                    }
                }
            }
        }
        // ѹ����
        int v, ret = 0;
        int new_w;
        for (auto& p : out_edges) {
            v = p.fs, new_w = w + p.sc;
            // û�м�֤·��/��֤·����Ҫ���ɵĽݾ�Ҫ��
            if (D_dist.find(v) == D_dist.end() || D_dist.find(v)->sc > new_w) {
                ++ret;
                if (!type && u != v) {
                    Connect(G[0][u], v, new_w);
                    addShortcut(u, x, v, new_w);
                    Connect(G[1][v], u, new_w);
                }
            }
        }
        return ret;
    }

    //�����������ڽڵ�x��y��ԭͼ�ľ���
    double getDis(int x, int y, int g) {
        if (x == y) return 0.0;
        for (auto& p : G[g][x]) {
            if (p.first == y)
                return p.second;
        }
        return INF;
    }


    // ���� CH ͼ
    void BuildG_CH() {
        G_CH.assign(2, Graph(n + 1, vector<Node>()));
        int v;
        int w;
        for (int u = 1; u <= n; ++u) {
            for (auto& p : G[0][u]) {
                v = p.fs;
                w = p.sc;
                if (imp[v] > imp[u])
                    G_CH[0][u].pb(mp(v, w));
                else
                    G_CH[1][v].pb(mp(u, w));
            }
        }
    }

    // ������·����;���Ľڵ� path:;���Ľڵ� mid:���·�����м�ڵ�
    void getNodeInPath(vector<int>& path, int mid) {
        for (int i = 0; i < 2; i++) {
            int x = mid;
            while (fa[i][x] != x) {
                x = fa[i][x];
                if (!i) {
                    path.insert(path.begin(), x);
                }
                else {
                    path.push_back(x);
                }
            }
            if (!i)
                path.push_back(mid);
        }
        for (int i = 1; i < path.size(); i++) {
            string u = to_string(path[i - 1]);
            string v = to_string(path[i]);
            string u_v = u + "?" + v;
            if (shortcut.find(u_v) != shortcut.end()) {
                path.insert(path.begin() + i, shortcut[u_v].begin() + 1, shortcut[u_v].end() - 1);
                // ����i,ָ��ԭi
                i += (shortcut[u_v].end() - 1) - (shortcut[u_v].begin() + 1);
            }
        }
    }

    // ��ȡ���·��
    int GetDistance(vector<int>& path) {
        dist[0][s] = dist[1][t] = 0;  // s Ϊ���, t Ϊ�յ�
        int SP = INF;                  // ���·�� shortest path
        pq[0].push(mp(0, s));
        pq[1].push(mp(0, t));
        Node front;
        int curr_node;
        int curr_dist;
        int min_node;  // ���·���Ľ����
        // �������յ�ֱ���� Dijkstra ���� (˫�� Dijkstra ����)
        while (!pq[0].empty() || !pq[1].empty()) {
            if (!pq[0].empty()) {
                front = pq[0].top();
                pq[0].pop();
                curr_node = front.sc;  // ��ǰ��� (�м���)
                curr_dist = front.fs;
                if (SP >= curr_dist)
                    RelaxNodeEdges(curr_node, 0);
                // ���ڽ��ֻ�ᱻ����һ�� ��˫��������������ظ�
                if (SP > dist[0][curr_node] + dist[1][curr_node]) {
                    SP = dist[0][curr_node] + dist[1][curr_node];
                    min_node = curr_node;
                }
            }
            if (!pq[1].empty()) {
                front = pq[1].top();
                pq[1].pop();
                curr_node = front.sc;
                curr_dist = front.fs;
                if (SP >= curr_dist)
                    RelaxNodeEdges(curr_node, 1);
                if (SP > dist[0][curr_node] + dist[1][curr_node]) {
                    SP = dist[0][curr_node] + dist[1][curr_node];
                    min_node = curr_node;
                }
            }
        }
        if (SP == INF)
            return -1;  // ������������·��, ���� -1
        getNodeInPath(path, min_node);
        return SP;
    }

    int GetDistanceWithoutPath() {
        dist[0][s] = dist[1][t] = 0;  // s Ϊ���, t Ϊ�յ�
        int SP = INF;                  // ���·�� shortest path
        pq[0].push(mp(0, s));
        pq[1].push(mp(0, t));
        Node front;
        int curr_node;
        int curr_dist;
        int min_node;  // ���·���Ľ����
        // �������յ�ֱ���� Dijkstra ���� (˫�� Dijkstra ����)
        while (!pq[0].empty() || !pq[1].empty()) {
            if (!pq[0].empty()) {
                front = pq[0].top();
                pq[0].pop();
                curr_node = front.sc;  // ��ǰ��� (�м���)
                curr_dist = front.fs;
                if (SP >= curr_dist)
                    RelaxNodeEdgesWithoutPath(curr_node, 0);
                // ���ڽ��ֻ�ᱻ����һ�� ��˫��������������ظ�
                if (SP > dist[0][curr_node] + dist[1][curr_node]) {
                    SP = dist[0][curr_node] + dist[1][curr_node];
                    min_node = curr_node;
                }
            }
            if (!pq[1].empty()) {
                front = pq[1].top();
                pq[1].pop();
                curr_node = front.sc;
                curr_dist = front.fs;
                if (SP >= curr_dist)
                    RelaxNodeEdgesWithoutPath(curr_node, 1);
                if (SP > dist[0][curr_node] + dist[1][curr_node]) {
                    SP = dist[0][curr_node] + dist[1][curr_node];
                    min_node = curr_node;
                }
            }
        }
        if (SP == INF)
            return -1;  // ������������·��, ���� -1
        return SP;
    }

    // �ɳڲ���
    void RelaxNodeEdges(int u, int g) {  // u: ��ǰ���, g: ��ǰͼ (0��ʾ����� 1��ʾ�����)
        int v;
        int w;
        for (auto& p : G_CH[g][u]) {
            v = p.fs, w = p.sc;
            if (dist[g][v] > dist[g][u] + w) {
                dist[g][v] = dist[g][u] + w;
                pq[g].push(mp(dist[g][v], v));
                fa[g][v] = u;
            }
        }
    }

    // �ɳڲ������洢���ڵ�
    void RelaxNodeEdgesWithoutPath(int u, int g) {  // u: ��ǰ���, g: ��ǰͼ (0��ʾ����� 1��ʾ�����)
        int v;
        int w;
        for (auto& p : G_CH[g][u]) {
            v = p.fs, w = p.sc;
            if (dist[g][v] > dist[g][u] + w) {
                dist[g][v] = dist[g][u] + w;
                pq[g].push(mp(dist[g][v], v));
            }
        }
    }

public:
    // CH ���캯��
    CH(string input) {
        cout << "Preprocessing...\n\n";
        read(input);
        preprocess();
        cout << "\nPreprocessing done!\n\n";
        car.remaining_power = 80.0;   //��ʼ����
        car.min_power = 30.0;       //������ֵ
        car.max_distance = 400.0;    //��������������
    }

    CH() {

    }

    // ��ѯ���·�������ұ���·��
    int Query(int _s, int _t, vector<int>& path) {  // path:���·��
        s = _s, t = _t;                           // ���������յ�
        dist.assign(2, vector<int>(n + 1, INF));   // ��ʼ������Ϊ�����
        // ��ʼ�����ڵ�Ϊ������
        fa.assign(2, vector<int>(n + 1, 0));
        for (int i = 0; i < n + 1; i++) {
            fa[0][i] = i;
            fa[1][i] = i;
        }
        pq.assign(2, DijkstraQueue());  // ��ʼ�����ȶ���
        return GetDistance(path);       // ��ȡ���·��
    }

    // ��ѯ���·�������Ҳ�����·��
    int QueryWithoutPath(int _s, int _t) {  // path:���·��
        s = _s, t = _t;                           // ���������յ�
        dist.assign(2, vector<int>(n + 1, INF));   // ��ʼ������Ϊ�����
        // ��ʼ�����ڵ�Ϊ������
        pq.assign(2, DijkstraQueue());  // ��ʼ�����ȶ���
        return GetDistanceWithoutPath();       // ��ȡ���·��
    }

    // ��ʾ�ݾ�
    void showcaseShortcut(ofstream& outFile) {
        long long shortcuts_cnt = shortcut.size();  // ��¼���ӵĽݾ���
        for (auto& x : shortcut) {
            cout << "Added shortcut: " << *(x.second.begin()) << " -> " << *(x.second.end() - 1) << " (weight: " << shortcut_weight[x.first] << ")" << endl;
            outFile << *(x.second.begin()) << " " << *(x.second.end() - 1) << " " << shortcut_weight[x.first] << endl;
        }
        cout << "Added " << shortcuts_cnt << " shortcuts:\n";
        outFile << "Added " << shortcuts_cnt << " shortcuts:\n";
        for (auto& x : shortcut) {
            cout << "	";
            outFile << "	";
            for (int i = 0; i < x.second.size(); i++) {
                if (i)
                    cout << " -> ";
                cout << x.second[i];
                if (i)
                    outFile << " -> ";
                outFile << x.second[i];
            }
            cout << endl;
            outFile << endl;
        }
        cout << endl;
        outFile << endl;
    }

    // Ԥ����׶λ��ǲ�Ҫʹ�õ�ǰʣ��������жϵ�ǰ�����Ƿ������ʻĳ·��
    bool isCanDriveThisWay(int distance) {
        int max_distance = car.max_distance * (100 - car.min_power) / 100;
        if (max_distance >= distance) {
            return true;
        }
        return false;
    }
};

//�ɴ���վ����ͼ
class CH_ChargeStation {

public:
    CH_ChargeStation() {
        // ����CH
        m = 0;
        G = CH("2000.txt");
        // Ԥ����chargeStations
        completeChargeStations();
        // Ԥ����isCanNocharge
        completeIs_can_no_charge();
        // Ԥ����nodeToStation��stationToStaion��nodeAroundStation��stationAroundNode
        completeNodeToStation();
        // Ԥ����stationToNode_shortcuts, stationToNode_shortcut_weight,stationToNode
        completeStationToNode();
        // Ԥ����stationToStation, stationToStation_shortcuts, stationToStation_shortcut_weight
        completeStationToStaion();
        // ���������ļ�
        generateInputFile();
        // ����CH_ChargeStation
        G_ChargeStation = CH("G_ChargeStation_input.txt");
        // Ԥ����nodeToNode_in_G_ChargeStation, nodeToNode_in_G_ChargeStation_shortcuts, nodeToNode_in_G_ChargeStation_shortcut_weight
        completeNodeToNode_in_G_ChargeStation();

    }

    CH G; // ԭͼ��CH
    CH G_ChargeStation; // �������յ㣬��;�ڵ��Ϊ���վ��ѹ��ͼ
    int m; // ����
    unordered_set<int> charge_stations; // ���վ��
    vector<vector<int>> is_can_no_charge; // ·�����Ƿ���Ҫ��� -1 : ��· -2 :�ɴﵫҪ���  ·��: �ɴ��Ҳ��ó��

    //unordered_map<string, vector<int>> nodeToStation_shortcuts; // ����Ԥ��������ͨ�ڵ㵽���վ�Ŀɴ�·��(δѹ��)
    unordered_map<string, int> nodeToStation_shortcut_weight; // ����Ԥ��������ͨ�ڵ㵽���վ�Ŀɴ�·����Ȩ��
    unordered_map<int, vector<int>> nodeToStation; // ������ͨ�ڵ���Χ�Ŀɴ���վ -- ����(node --> station), key -- node

    //unordered_map<string, vector<int>> stationToNode_shortcuts; // ����Ԥ�����г��վ����ͨ�ڵ�Ŀɴ�·��(δѹ��)
    unordered_map<string, int> stationToNode_shortcut_weight; // ����Ԥ�����г��վ����ͨ�ڵ�Ŀɴ�·����Ȩ��
    unordered_map<int, vector<int>> stationToNode; // ������ͨ�ڵ���Χ�Ŀɴ���վ -- ����(node <-- station), key -- node

    unordered_map<string, vector<int>> stationToStation_shortcuts; // ����Ԥ�����г��վ�����վ�Ŀɴ�·��(δѹ��)
    unordered_map<string, int> stationToStation_shortcut_weight; // ����Ԥ�����г��վ�����վ�Ŀɴ�·����Ȩ��
    unordered_map<int, vector<int>> stationToStation; // ������վ��Χ�Ŀɴ���վ

    //unordered_map<string, vector<int>> nodeToNode_in_G_ChargeStation_shortcuts; // ����G_ChargeStationͼ�нڵ㵽�ڵ��·��(δѹ��)
    unordered_map<string, int> nodeToNode_in_G_ChargeStation_shortcut_weight; // ����G_ChargeStationͼ�нڵ㵽�ڵ�·��Ȩ��
    unordered_map<int, vector<int>> nodeToNode_in_G_ChargeStation; // ����G_ChargeStationͼ�нڵ㵽�ڵ�

    // Ԥ����chargeStations
    void completeChargeStations() {
        cout << "completeChargeStations begin" << endl;
        for (int i = 0, I = G.stations.size(); i < I; i++) {
            charge_stations.insert(G.stations[i]);
        }
        cout << "completeChargeStations end" << endl;
        return;
    }

    // Ԥ����isCanNocharge
    void completeIs_can_no_charge() {
        cout << "completeIs_can_no_charge begin" << endl;
        is_can_no_charge.assign(G.n + 1, vector<int>(G.n + 1, -1));
        for (int i = 1; i <= G.n; i++) {
            for (int j = 1; j <= G.n; j++) {
                if (i == j)continue;
                int weight = G.QueryWithoutPath(i, j);
                if (weight == -1) {
                    is_can_no_charge[i][j] = -1;
                }
                else if (G.isCanDriveThisWay(weight)) {
                    is_can_no_charge[i][j] = weight;
                }
                else {
                    is_can_no_charge[i][j] = -2;
                }
            }
        }
        cout << "completeIs_can_no_charge end" << endl;
        return;
    }

    // Ԥ����nodeToStation,nodeToStation_shortcuts,nodeToStation_shortcut_weight
    void completeNodeToStation() {
        cout << "completeNodeToStation begin" << endl;
        for (int i = 1; i <= G.n; i++) {
            // �ж��ǲ��ǳ��վ
            if (charge_stations.find(i) != charge_stations.end()) continue;
            for (int j = 0, J = G.stations.size(); j < J; j++) {
                int weight = G.QueryWithoutPath(i, G.stations[j]);
                if (weight == -1) continue;
                // �ж��������ʻ��̣�����������ֵ���ɴ�
                if (G.isCanDriveThisWay(weight)) {
                    nodeToStation[i].push_back(G.stations[j]);
                    string str_node = to_string(i);
                    string str_station = to_string(G.stations[j]);
                    string node_station = str_node + "?" + str_station;
                    nodeToStation_shortcut_weight[node_station] = weight;
                }
            }
        }
        cout << "completeNodeToStation end" << endl;
        return;
    }

    // Ԥ����stationToNode_shortcuts, stationToNode_shortcut_weight,stationToNode
    void completeStationToNode() {
        cout << "completeStationToNode beign" << endl;
        for (int i = 0, I = G.stations.size(); i < I; i++) {
            for (int j = 1; j <= G.n; j++) {
                // �ж��ǲ��ǳ��վ
                if (charge_stations.find(j) != charge_stations.end()) continue;
                int weight = G.QueryWithoutPath(G.stations[i], j);
                if (weight == -1) continue;
                if (G.isCanDriveThisWay(weight)) {
                    stationToNode[j].push_back(G.stations[i]);
                    string str_station = to_string(G.stations[i]);
                    string str_node = to_string(j);
                    string station_node = str_station + "?" + str_node;
                    stationToNode_shortcut_weight[station_node] = weight;
                }
            }
        }
        cout << "completeStationToNode end" << endl;
        return;
    }

    // Ԥ����stationToStaion, stationToStation_shortcuts, stationToStation_shortcut_weight
    void completeStationToStaion() {
        cout << "completeStationToStaion begin" << endl;
        for (int i = 0, I = G.stations.size(); i < I; i++) {
            for (int j = 0, J = G.stations.size(); j < J; j++) {
                // �ж��ǲ�����ͬ���վ
                if (i == j) continue;
                vector<int> path;
                int weight = G.Query(G.stations[i], G.stations[j], path);
                if (weight == -1) continue;
                if (G.isCanDriveThisWay(weight)) {
                    stationToStation[G.stations[i]].push_back(G.stations[j]);
                    m++;
                    string str_station1 = to_string(G.stations[i]);
                    string str_station2 = to_string(G.stations[j]);
                    string station1_stations2 = str_station1 + "?" + str_station2;
                    stationToStation_shortcuts[station1_stations2] = path;
                    stationToStation_shortcut_weight[station1_stations2] = weight;
                }
            }
        }
        cout << "completeStationToStaion end" << endl;
        return;
    }

    // ���������ļ�(����G_ChargeStation)
    void generateInputFile() {
        cout << "generateInputFile begin" << endl;
        ofstream outFile("G_ChargeStation_input.txt");
        if (!outFile) {
            cerr << "�޷����ļ� G_ChargeStation_input.txt" << endl;
            return;
        }
        // ����ڵ���������
        outFile << G.n << ' ' << m << endl;
        // ����ߺ�Ȩֵ
        for (auto& x : stationToStation_shortcut_weight) {
            int index = x.first.find('?');
            outFile << x.first.substr(0, index) << ' ' << x.first.substr(index + 1) << ' ' << x.second << endl;
        }
        // ������վ�����ͱ��
        outFile << G.stations.size() << endl;
        for (int i = 0, I = G.stations.size(); i < I; i++) {
            outFile << G.stations[i] << endl;
        }
        cout << "generateInputFile endl" << endl;
        return;
    }

    // Ԥ����nodeToNode_in_G_ChargeStation, nodeToNode_in_G_ChargeStation_shortcuts, nodeToNode_in_G_ChargeStation_shortcut_weight
    void completeNodeToNode_in_G_ChargeStation() {
        cout << "completeNodeToNode_in_G_ChargeStation begin" << endl;
        for (int i = 0, I = G_ChargeStation.stations.size(); i < I; i++) {
            for (int j = 0, J = G_ChargeStation.stations.size(); j < J; j++) {
                if (i == j) continue;
                int weight = G_ChargeStation.QueryWithoutPath(G_ChargeStation.stations[i], G_ChargeStation.stations[j]);
                if (weight == -1)continue;
                nodeToNode_in_G_ChargeStation[G.stations[i]].push_back(G.stations[j]);
                string str_station1 = to_string(G.stations[i]);
                string str_station2 = to_string(G.stations[j]);
                string station1_stations2 = str_station1 + "?" + str_station2;
                nodeToNode_in_G_ChargeStation_shortcut_weight[station1_stations2] = weight;
            }
        }
        // ����·��ֻ����һ�����վ
        nodeToNode_in_G_ChargeStation_shortcut_weight[""] = 0;
        cout << "completeNodeToNode_in_G_ChargeStation end" << endl;
    }

    // �����վ�����վ�Ľݾ���ԭ
    void decompressPath(vector<int>& path) {
        for (int i = 1; i < path.size(); i++) {
            string u = to_string(path[i - 1]);
            string v = to_string(path[i]);
            string u_v = u + "?" + v;
            if (stationToStation_shortcuts.find(u_v) != stationToStation_shortcuts.end()) {
                path.insert(path.begin() + i, stationToStation_shortcuts[u_v].begin() + 1, stationToStation_shortcuts[u_v].end() - 1);
                // ����i,ָ��ԭi
                i += (stationToStation_shortcuts[u_v].end() - 1) - (stationToStation_shortcuts[u_v].begin() + 1);
            }
        }
        return;
    }

    // ���ǵ�����·������
    int Query(int s, int t, vector<int>& path) {
        if (is_can_no_charge[s][t] == -1) {
            return -1;
        }
        else if (is_can_no_charge[s][t] != -2 && G.car.distanceCanRun() >= is_can_no_charge[s][t]) {
            return G.Query(s, t, path);
        }
        else {
            // ���s��t���ǳ��վ
            if (charge_stations.find(s) != charge_stations.end() && charge_stations.find(t) != charge_stations.end()) {
                string s_t = to_string(s) + "?" + to_string(t);
                if (nodeToNode_in_G_ChargeStation_shortcut_weight.find(s_t) != nodeToNode_in_G_ChargeStation_shortcut_weight.end()) {
                    G_ChargeStation.Query(s, t, path);
                    decompressPath(path);
                    return nodeToNode_in_G_ChargeStation_shortcut_weight[s_t];
                }
                return -1;
            }
            // ���t�ǳ��վ,s����
            else if (charge_stations.find(s) == charge_stations.end() && charge_stations.find(t) != charge_stations.end()) {
                // ɸѡs�ܵ���ĳ��վ
                int sum_weight = INF;
                int x_node = -1;
                string s_x_ = "", x_t_ = "";
                for (auto& x : nodeToStation[s]) {
                    string s_x = to_string(s) + "?" + to_string(x);
                    int weight_s_x = nodeToStation_shortcut_weight[s_x];
                    // Ҫ������ʼ����
                    if (weight_s_x > G.car.distanceCanRun())continue;
                    string x_t;
                    // ������ֻ����һ�����վ�������ӦΪ""
                    if (x == t) x_t = "";
                    else x_t = to_string(x) + "?" + to_string(t);
                    int weight_x_t = 0;
                    if (nodeToNode_in_G_ChargeStation_shortcut_weight.find(x_t) != nodeToNode_in_G_ChargeStation_shortcut_weight.end()) {
                        weight_x_t = nodeToNode_in_G_ChargeStation_shortcut_weight[x_t];
                    }
                    else continue;
                    if (sum_weight > weight_s_x + weight_x_t) {
                        sum_weight = weight_s_x + weight_x_t;
                        s_x_ = s_x;
                        x_t_ = x_t;
                        x_node = x;
                    }
                }
                if (sum_weight != INF) {
                    // ������·��ƴ��
                    if (x_t_ == "") {
                        G.Query(s, t, path);
                    }
                    else {
                        G.Query(s, x_node, path);
                        vector<int> path1;
                        G_ChargeStation.Query(x_node, t, path1);
                        decompressPath(path1);
                        path.insert(path.end(), path1.begin() + 1, path1.end());
                    }
                    return sum_weight;
                }
                return -1;
            }
            // ���s�ǳ��վ,t����
            else if (charge_stations.find(s) != charge_stations.end() && charge_stations.find(t) == charge_stations.end()) {
                // ɸѡt�ܵ���ĳ��վ
                int sum_weight = INF;
                int x_node = -1;
                string s_x_ = "", x_t_ = "";
                for (auto& x : stationToNode[t]) {
                    string x_t = to_string(x) + "?" + to_string(t);
                    int weight_x_t = stationToNode_shortcut_weight[x_t];
                    string s_x;
                    if (s == x) s_x = "";
                    else s_x = to_string(s) + "?" + to_string(x);
                    int weight_s_x = 0;
                    if (nodeToNode_in_G_ChargeStation_shortcut_weight.find(s_x) != nodeToNode_in_G_ChargeStation_shortcut_weight.end()) {
                        weight_s_x = nodeToNode_in_G_ChargeStation_shortcut_weight[s_x];
                    }
                    else continue;
                    if (sum_weight > weight_s_x + weight_x_t) {
                        sum_weight = weight_s_x + weight_x_t;
                        s_x_ = s_x;
                        x_t_ = x_t;
                        x_node = x;
                    }
                }
                if (sum_weight != INF) {
                    // ������·��ƴ��
                    if (s_x_ == "") {
                        G.Query(s, t, path);
                    }
                    else {
                        G_ChargeStation.Query(s, x_node, path);
                        decompressPath(path);
                        vector<int> path1;
                        G.Query(x_node, t, path1);
                        path.insert(path.end(), path1.begin() + 1, path1.end());
                    }
                    return sum_weight;
                }
                return -1;
            }
            // ���s��t�����ǳ��վ
            else {
                int sum_weight = INF;
                string s_x_ = "", x_y_ = "", y_t_ = "";
                int x_node = -1, y_node = -1;
                for (auto& x : nodeToStation[s]) {
                    // ȷ�����վx
                    string s_x = to_string(s) + "?" + to_string(x);
                    int weight_s_x = nodeToStation_shortcut_weight[s_x];
                    if (weight_s_x > G.car.distanceCanRun()) continue;
                    for (auto& y : stationToNode[t]) {
                        // ȷ�����վy
                        string y_t = to_string(y) + "?" + to_string(t);
                        int weight_y_t = stationToNode_shortcut_weight[y_t];
                        string x_y;
                        if (x == y) x_y = "";
                        else x_y = to_string(x) + "?" + to_string(y);
                        int weight_x_y = 0;
                        if (nodeToNode_in_G_ChargeStation_shortcut_weight.find(x_y) != nodeToNode_in_G_ChargeStation_shortcut_weight.end()) {
                            weight_x_y = nodeToNode_in_G_ChargeStation_shortcut_weight[x_y];
                        }
                        else continue;
                        if (sum_weight > weight_s_x + weight_x_y + weight_y_t) {
                            sum_weight = weight_s_x + weight_x_y + weight_y_t;
                            s_x_ = s_x;
                            x_y_ = x_y;
                            y_t_ = y_t;
                            x_node = x;
                            y_node = y;
                        }
                    }
                }
                if (sum_weight != INF) {
                    // ������·��ƴ��
                    if (x_y_ == "") {
                        G.Query(s, x_node, path);
                        vector<int> path1;
                        G.Query(x_node, t, path1);
                        path.insert(path.end(), path1.begin() + 1, path1.end());
                    }
                    else {
                        G.Query(s, x_node, path);
                        vector<int> path1;
                        G_ChargeStation.Query(x_node, y_node, path1);
                        decompressPath(path1);
                        path.insert(path.end(), path1.begin() + 1, path1.end());
                        path1.clear();
                        G.Query(y_node, t, path1);
                        path.insert(path.end(), path1.begin() + 1, path1.end());
                    }
                    return sum_weight;
                }
                return -1;
            }
        }
    }
};



int main() {
    ios::sync_with_stdio(0);  // �������������
    int T = clock();
    CH_ChargeStation CC;
    // // ����һ��ofstream���󣬲���һ����Ϊ"output.txt"���ļ�
    // std::ofstream outFile("output.txt");
    // ����ļ��Ƿ�ɹ���
    if (!outFile) {
        std::cerr << "�޷����ļ� output.txt" << std::endl;
        return 1;
    }

    // ��ʾ�ݾ�
    CC.G.showcaseShortcut(outFile);
    CC.G_ChargeStation.showcaseShortcut(outFile);

    cout << "Preprocessing time: " << 1.0 * (clock() - T) / CLOCKS_PER_SEC << "s\n"
        << endl;
    outFile << "Preprocessing time: " << 1.0 * (clock() - T) / CLOCKS_PER_SEC << "s\n"
        << endl;

    int q, s, t;
    double tot_time = 0, query_time;
    queries >> q;
    for (int i = 0; i < q; ++i) {
        queries >> s >> t;
        T = clock();

        vector<int> path;
        int dis = CC.Query(s, t, path);
        cout << "Shortest path distance from " << s << " to " << t << " : " << dis << endl;
        cout << "	";
        for (int i = 0; i < path.size(); i++) {
            if (i)
                cout << " -> ";
            cout << path[i];
        }
        cout << endl;

        outFile << "Shortest path distance from " << s << " to " << t << " : " << dis << endl;
        outFile << "	";
        for (int i = 0; i < path.size(); i++) {
            if (i)
                outFile << " -> ";
            outFile << path[i];
        }
        outFile << endl;
        query_time = 1.0 * (clock() - T) / CLOCKS_PER_SEC;
        outFile << "Time: " << query_time << "s\n\n";
        cout << "Time: " << query_time << "s\n\n";
        tot_time += query_time;
    }
    outFile << "\nAverage query time: " << tot_time / q << "s\n";
    outFile.close();
    cout << "\nAverage query time: " << tot_time / q << "s\n";
    cout << "Press Enter to terminate...";
    cin.ignore();  // ��ͣ����
    string term;
    getline(cin, term);
    return 0;
}


