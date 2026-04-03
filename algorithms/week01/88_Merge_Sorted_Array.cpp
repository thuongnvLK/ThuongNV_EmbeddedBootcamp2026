#include <bits/stdc++.h>

using namespace std;

#define SOLUTION_2
class Solution {

public:
    void merge(vector<int>& nums1, int m, vector<int>& nums2, int n) {
        #ifdef SOLUTION_1
        int last = m + n - 1;
        int i = m - 1;
        int j = n - 1;

        while (j >= 0) {
            if (i >= 0 && nums1[i] > nums2[j]) {
                nums1[last--] = nums1[i--]; 
            } else {
                nums1[last--] = nums2[j--];
            }
        }
        #endif

        #ifdef SOLUTION_2
            int last = m +n - 1;

            while (m > 0 && n > 0) {
                if (nums1[m - 1] > nums2[n - 1]) {
                    nums1[last] = nums1[m - 1];
                    m--;
                } else {
                    nums1[last] = nums2[n - 1];
                    n--;
                }

                last--;
            }

            while (n > 0) {
                nums1[last] = nums2[n - 1];
                n--;
                last--;
            }
        #endif

    }

};

int main() {

    Solution so;

    vector<int> nums1 = {1,2,3,0,0,0};
    vector<int> nums2 = {2,5,6};
    int m = 3;
    int n = 3;

    so.merge(nums1, m, nums2, n);

    for (int i = 0; i < nums1.size(); i++) {
        cout << nums1[i] << endl;
    }
    

    return 0;
}