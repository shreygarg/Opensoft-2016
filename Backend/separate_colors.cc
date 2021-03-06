#include "opencv2/opencv.hpp"
#include <iostream>
#include <map>
#include <vector>
#include <queue>
#include <algorithm>

using namespace std;
using namespace cv;

const int HSV_THRESH = 15;
RNG rng(12345);

const int dx[] = {-1, -1, -1, 0, 0, +1, +1, +1};
const int dy[] = {-1, 0, +1, -1, +1, -1, 0, +1};

inline bool isBlack(int h, int s, int v) {
    return s <= 30;
}

inline bool isWhite(int h, int s, int v) {
    return h <= HSV_THRESH and s <= HSV_THRESH and v >= 100;
}

void bfs(const Mat& img, vector < vector <int> >& visited, int x, int y, int hVal, int colorId) {
    queue < pair <int, int> > q;
    q.push(make_pair(x, y));
    visited[x][y] = colorId;
    while (!q.empty()) {
        x = q.front().first;
        y = q.front().second;
        q.pop();
        for (int k = 0; k < 8; ++k) {
            int nx = x + dx[k];
            int ny = y + dy[k];
            if (nx >= 0 and nx < img.rows and ny >= 0 and ny < img.cols and visited[nx][ny] == -1) {
                Vec3b val = img.at<Vec3b>(nx, ny);
                int h = val.val[0];
                int s = val.val[1];
                int v = val.val[2];

                if (isWhite(h, s, v)) {
                    // white
                    continue;
                }

                if (isBlack(h, s, v)) {
                    // black
                    continue;
                }

                if (abs(h - hVal) <= HSV_THRESH) {
                    visited[nx][ny] = colorId;
                    q.push(make_pair(nx, ny));
                }
            }
        }
    }
}

void removeIsolated(Mat& img) {
    for (int x = 0; x < img.rows; ++x) {
        for (int y = 0; y < img.cols; ++y) {
            int val = img.at<uchar>(x, y);
            if (val > 200) {
                int countWhiteNeighs = 0;
                for (int k = 0; k < 8; ++k) {
                    int nx = x + dx[k];
                    int ny = y + dy[k];
                    if (nx >= 0 and nx < img.rows and ny >= 0 and ny < img.cols) {
                        if (img.at<uchar>(nx, ny) > 200)
                            ++countWhiteNeighs;
                    }
                }
                if (countWhiteNeighs <= 2) {
                    img.at<uchar>(x, y) = 0;
                }
            }
        }
    }
}

void erode(Mat& img) {
    vector < pair <int, int> > toBlack;
    for (int x = 0; x < img.rows; ++x) {
        for (int y = 0; y < img.cols; ++y) {
            if (img.at<uchar>(x, y) < 200)
                continue;
            int cnt = 0;
            for (int dx = -1; dx <= +1; ++dx) {
                for (int dy = -1; dy <= +1; ++dy) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 and nx < img.rows and ny >= 0 and ny < img.cols) {
                        if (img.at<uchar>(nx, ny) > 200)
                            ++cnt;
                    }
                }
            }
            if (cnt <= 5)
                toBlack.push_back(make_pair(x, y));
        }
    }
    for (int i = 0; i < toBlack.size(); ++i) {
        img.at<uchar>(toBlack[i].first, toBlack[i].second) = 0;
    }
}

void dilate(Mat& img) {
    vector < pair <int, int> > toWhite;
    for (int x = 0; x < img.rows; ++x) {
        for (int y = 0; y < img.cols; ++y) {
            if (img.at<uchar>(x, y) > 200)
                continue;
            int cnt = 0;
            for (int dx = -1; dx <= +1; ++dx) {
                for (int dy = -1; dy <= +1; ++dy) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 and nx < img.rows and ny >= 0 and ny < img.cols) {
                        if (img.at<uchar>(nx, ny) > 200)
                            ++cnt;
                    }
                }
            }
            if (cnt >= 2)
                toWhite.push_back(make_pair(x, y));
        }
    }
    for (int i = 0; i < toWhite.size(); ++i) {
        img.at<uchar>(toWhite[i].first, toWhite[i].second) = 255;
    }
}

struct Data {
    int x, y;
    int w;

    Data(int x, int y, int w) : x(x), y(y), w(w) {}

    bool operator < (const Data& o) const {
        return w > o.w;
    }
};

const int MAX_DIST = 50;

void dijkstra(Mat& img, const pair <int, int>& src, const pair <int, int>& dst) {
    if (abs(src.first - dst.first) * abs(src.first - dst.first) + abs(src.second - dst.second) * abs(src.second - dst.second) <= 5000)
        line(img, Point(src.second, src.first), Point(dst.second, dst.first), Scalar(255), 1);

    /*
    // cout<<src.first<<" "<<src.second<<"  "<<dst.first<<" "<<dst.second<<endl;
    priority_queue <Data> Q;
    
    map < pair <int, int>, bool> visited;
    map < pair <int, int>, int> dist;
    map < pair <int, int>, pair <int, int> > parent;
    
    Q.push (Data(src.first, src.second, 0));
    dist[src] = 0;
    parent[src] = make_pair(-1, -1);

    while (!Q.empty()) {
        Data p = Q.top();
        Q.pop();

        if (visited.count(make_pair(p.x, p.y)))
            continue;
        
        visited[make_pair(p.x, p.y)] = true;

        if (p.x == dst.first and p.y == dst.second) {
            break;
        }

        int x = p.x;
        int y = p.y;
        int d = dist[make_pair(x, y)];

        for (int k = 0; k < 8; ++k) {
            int nx = x + dx[k];
            int ny = y + dy[k];
            pair <int, int> q(nx, ny);
            if (nx >= 0 and nx < img.rows and ny >= y and ny < img.cols) {
                if (visited.count(q))
                    continue;
                if (abs(nx - src.first) <= MAX_DIST and abs(ny - src.second) <= MAX_DIST) {
                    if (!dist.count(q)) {
                        dist[q] = d + 1;
                        Q.push(Data(nx, ny, d + 1));
                        parent[q] = make_pair(x, y);
                    } else if (d + 1 < dist[q]) {
                        dist[q] = d + 1;
                        Q.push(Data(nx, ny, d + 1));
                        parent[q] = make_pair(x, y);
                    }
                }
            }
        }
    }


    int x, y;

    auto p = parent[{x, y}];
    int i = dst.first;
    int j = dst.second;
    if (!parent.count(dst))
        return;
    
    while(i != src.first && j != src.second){
        assert(parent.count({i, j}));
        img.at<uchar>(i, j) = 255;
        i = parent[{i,j}].first;
        j = parent[{i,j}].second;
    }

    */
}

void fillGaps(Mat& img) {
    vector <int> temp(img.cols);
    int a = -1, b = -1;
    for (int y = 0; y < img.cols; ++y)
            temp[y] = 0;

    vector < pair < pair <int, int>, pair <int, int> > > toFill;
    for (int y = 0; y < img.cols; ++y) {
        for (int x = 0; x < img.rows; ++x) {
            if(img.at<uchar>(x,y) > 200 && y > 0){
                temp[y] = 1;
                if(temp[y-1] == 1) {
                    a = x;
                    b = y;
                } else if (a != -1 and b != -1) {
                    toFill.push_back({{a, b}, {x, y}});
                }
            }
       }
    }

    for (int i = 0; i < toFill.size(); ++i) {
        dijkstra(img, toFill[i].first, toFill[i].second);
    }
}

void showContours(Mat& src_gray) {
    blur( src_gray, src_gray, Size(3,3) );
    static string str = "a";
    Mat threshold_output;
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;

    /// Detect edges using Threshold
    threshold( src_gray, threshold_output, 100, 255, THRESH_BINARY );
    /// Find contours
    findContours( threshold_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

    /// Approximate contours to polygons + get bounding rects and circles
    vector<vector<Point> > contours_poly( contours.size() );
    vector<Rect> boundRect( contours.size() );
    vector<Point2f>center( contours.size() );
    vector<float>radius( contours.size() );

    for( int i = 0; i < contours.size(); i++ )
     { approxPolyDP( Mat(contours[i]), contours_poly[i], 3, true );
       boundRect[i] = boundingRect( Mat(contours_poly[i]) );
       minEnclosingCircle( (Mat)contours_poly[i], center[i], radius[i] );
     }


    /// Draw polygonal contour + bonding rects + circles
    Mat drawing = Mat::zeros( threshold_output.size(), CV_8UC3 );
    for( int i = 0; i< contours.size(); i++ )
     {
       Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
       drawContours( drawing, contours_poly, i, color, 1, 8, vector<Vec4i>(), 0, Point() );
       rectangle( drawing, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0 );
       circle( drawing, center[i], (int)radius[i], color, 2, 8, 0 );
     }

    /// Show in a window
    namedWindow( "Contours_" + str, CV_WINDOW_NORMAL);
    imshow( "Contours_" + str, drawing );
    str[0]++;
}


void separateColors(Mat& img) {
    Mat dst;
    cvtColor(img, img, CV_BGR2HSV);
    namedWindow("Input Image", WINDOW_NORMAL);
    imshow("Input Image", img);

    // namedWindow("Output Image", WINDOW_NORMAL);
    // imshow("Output Image", dst);

    map <uchar, int> hueCount;
    for (int x = 0; x < img.rows; ++x) {
        for (int y = 0; y < img.cols; ++y) {
            Vec3b val = img.at<Vec3b>(x, y);
            int h = val.val[0];
            int s = val.val[1];
            int v = val.val[2];

            if (isWhite(h, s, v)) {
                // white
                continue;
            }

            if (isBlack(h, s, v)) {
                // black
                continue;
            }

            hueCount[val.val[0]]++;
        }
    }

    vector < pair <int, int> > hueVals;
    for (auto p : hueCount) {
        hueVals.push_back(make_pair(p.second, p.first));
    }

    sort(hueVals.rbegin(), hueVals.rend());

    for (int i = 0; i < 30 and i < hueVals.size(); ++i) {
        printf("i : %d [%d]\n", i, hueVals[i].second);
    }

    vector < vector <int> > visited(img.rows, vector <int> (img.cols, -1));
    int colorId = 0;
    map <int, int> idxMap;

    for (int x = 0; x < img.rows; ++x) {
        for (int y = 0; y < img.cols; ++y) {
            if (visited[x][y] != -1)
                continue;

            Vec3b val = img.at<Vec3b>(x, y);
            int h = val.val[0];
            int s = val.val[1];
            int v = val.val[2];

            if (isWhite(h, s, v)) {
                // white
                continue;
            }

            if (isBlack(h, s, v)) {
                // black
                continue;
            }

            int idx = -1;
            for (int i = 0; i < hueVals.size(); ++i) {
                if (abs(hueVals[i].second - h) <= HSV_THRESH) {
                    idx = i;
                    break;
                }
            }
            if (!idxMap.count(idx)) {
                printf("idx : %d colorId : %d [%d, %d, %d] at (%d, %d)\n", idx, colorId, hueVals[idx].second, s, v, x, y);
                idxMap[idx] = colorId++;
            }

            bfs(img, visited, x, y, hueVals[idx].second, idxMap[idx]);
        }
    }

    map <int, int> eqClass;
    for (int i = 0; i < hueVals.size(); ++i) {
        if (idxMap.count(i)) {
            for (int j = 0; j <= i; ++j) if (idxMap.count(j)) {
                if (abs(hueVals[i].second - hueVals[j].second) <= HSV_THRESH) {
                    printf("same %d and %d\n", i, j);
                    eqClass[idxMap[i]] = idxMap[j];
                    break;
                }
            }
        }
    }
    cout << idxMap.size() << "\n";
    vector <Mat> separated(idxMap.size());
    for (int i = 0; i < separated.size(); ++i) {
        separated[i] = Mat(img.rows, img.cols, CV_8UC1, Scalar(0));
    }

    vector <int> cnt(separated.size(), 0);
    for (int x = 0; x < img.rows; ++x) {
        for (int y = 0; y < img.cols; ++y) {
            if (visited[x][y] == -1)
                continue;
            int id = eqClass[visited[x][y]];
            separated[id].at<uchar>(x, y) = 255;
            ++cnt[id];
        }
    }

    const Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));

    for (int i = 0; i < separated.size(); ++i) {
        if (cnt[i] < 150)
            continue;
        string win(1, 'a' + i);
        erode(separated[i]);
        dilate(separated[i]);
        // morphologyEx(separated[i], separated[i], MORPH_OPEN, kernel, Point(-1,-1), 1);
        erode(separated[i]);
        fillGaps(separated[i]);
        
        namedWindow(win, WINDOW_NORMAL);
        imshow(win, separated[i]);
        // showContours(separated[i]);
        imwrite(win + ".png", separated[i]);
    }
}

int main(int argc, char const *argv[]) {
    if( argc != 2) {
        cout <<"Provide image file." << endl;
        return -1;
    }

    Mat image = imread(argv[1], CV_LOAD_IMAGE_COLOR);

    if(!image.data) {
        cout <<  "Could not open or find the image" << std::endl ;
        return -1;
    }
    separateColors(image);
    waitKey(0);
    destroyAllWindows();
    return 0;
}