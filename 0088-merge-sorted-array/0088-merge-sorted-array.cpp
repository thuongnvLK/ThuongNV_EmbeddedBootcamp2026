class Solution {
public:
    void merge(vector<int>& nums1, int m, vector<int>& nums2, int n) {
        int i = m - 1;  // m = 0 -> i = -1
        int j = n - 1; // n = 1 -> j = 0
        int k = m + n -1; // k = 0

        while ((i >= 0) && (j >= 0)) { // sai
            if (nums1[i] > nums2[j]) {
                nums1[k--] = nums1[i--]; 
            } else {
                nums1[k--] = nums2[j--];
            }
        }

        while (j >= 0) { 
            nums1[k--] = nums2[j--];
        }
    }
};