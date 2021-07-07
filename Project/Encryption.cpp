#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <bitset>
#include <string>
#include <iostream>
#include <math.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <array>
#include <iterator>
#include "compressor.h"
using namespace cv;
using namespace std;
#pragma region Variable

string imagePath;//link ảnh cần mã hóa hoặc giải mã
string outputPath;//link ảnh sau khi mã hóa hoặc giải mã
// 1 -> encrypt , 2 -> decrypt
int operation;

//parameter cho logistic và henon map
double parameters[6];


vector<pair<double, int >> x; //vecto lưu giá trị <giá trị X trong logisticmap, stt>
vector<pair<double, int >> y; //vecto lưu giá trị <giá trị y trong logisticmap, stt>
Vec<unsigned char, 3>  pixel;
#pragma endregion Variable

#pragma region BGR
//chuyển giá trị ảnh BGR về dạng nhị phân (bitset)
bitset<24> BgrToBinary(int blue, int green, int red)
{
	return bitset<24>(bitset<8>(blue).to_string() + bitset<8>(green).to_string() + bitset<8>(red).to_string());
}

//chuyển BGR về dạng số nguyên lưu vào mảng array(b,g,r)
array<int, 3> extractBGR(bitset<24> pixel)
{
	string temp;
	int i = 23;

	while (i > 15)
		temp += pixel[i--] ? "1" : "0";
	//blue từ bit 16->23
	int blue = (bitset<8>(temp).to_ulong());
	temp.clear();
	//green từ bit 8->15
	while (i > 7)
		temp += pixel[i--] ? "1" : "0";

	int green = (bitset<8>(temp).to_ulong());
	temp.clear();
	//red từ bit 0->7
	while (i >= 0)
		temp += pixel[i--] ? "1" : "0";

	int red = (bitset<8>(temp).to_ulong());
	temp.clear();
	return { blue , green , red };
}
#pragma endregion BGR

#pragma region Encrypt
//Hoán vị dựa theo logistic map
void Permutation(Mat& img)
{
	//Mat img = imread(imagePath);
	imshow("Input image", img);
	//khai báo biến
	double temp;
	// Tạo X seq ngẫu nhiên dưa vào Logistic map với số phần tử bằng chiều rộng ảnh
	for (int i = 0; i < img.cols; ++i)
	{
		//Dựa vào logistic tạo chuỗi ngẫu nhiên
		temp = parameters[1] = parameters[0] * parameters[1] * (1 - parameters[1]);
		x.push_back({ temp,i });

	}
	// Tạo Y seq ngẫu nhiên dưa vào Logistic map với số phần tử bằng chiều cao ảnh
	for (int i = 0; i < img.rows; ++i)
	{
		//Dựa vào logistic tạo chuỗi ngẫu nhiên
		temp = parameters[1] = parameters[0] * parameters[1] * (1 - parameters[1]);
		y.push_back({ temp,i });

	}
	//Sắp xếp giá trị không giảm
	std::sort(x.begin(), x.end());
	std::sort(y.begin(), y.end());
	int v = 0;
	//Thực hiện hoán vị dựa vào x
	for (int i = 0; i < img.rows; ++i)
	{
		for (int j = 0; j < img.cols; ++j)
		{
			if (v >= img.cols)
				v = 0;
			int temp = x[v].second;
			pixel = img.at<Vec3b>(i, temp);
			img.at<Vec3b>(i, temp) = img.at<Vec3b>(i, j);
			img.at<Vec3b>(i, j) = pixel;
			v++;
		}
	}
	v = 0;
	//Thực hiện hoán vị dựa vào y
	for (int i = 0; i < img.rows; ++i)
	{
		for (int j = 0; j < img.cols; ++j)
		{
			if (v >= img.rows)
				v = 0;
			int temp = y[v].second;
			pixel = img.at<Vec3b>(temp, j);
			img.at<Vec3b>(temp, j) = img.at<Vec3b>(i, j);
			img.at<Vec3b>(i, j) = pixel;
			v++;
		}
	}
	imshow("Permutation", img);
}
//Công thức HenonMap
double HenonMap()
{
	// x_n+1=1-a*x^2_n+y_n
	// y_n+1=b*x_n
	double result = 1 - (parameters[4] * pow(parameters[3], 2.0)) + parameters[2];
	parameters[2] = parameters[5] * parameters[3];
	return result;
}
//Khuếch tán dựa theo HenonMap
void Diffusion(Mat& img)
{
	// Khai báo biến
	bitset<24> LSB_2;//key dựa vào henon
	bitset<24> rgbConversion; //điểm ảnh có giá trị rgb dưới dạng nhị phân
	bitset<24> encryptedPixel;//điểm ảnh sau khi encrypt
	bitset<24> xorVal;//Biến tạm

	array<int, 3> BGR;//mảng giá trị màu sau khi encrypt

	uint64_t u_2;

	int blue;
	int green;
	int red;

	//Xét từng pixel của ảnh
	for (int i = 0; i < img.rows; i++)
	{
		for (int j = 0; j < img.cols; j++)
		{
			//lưu ảnh theo BGR
			Vec3b intensity = img.at<Vec3b>(i, j);

			blue = intensity.val[0];
			green = intensity.val[1];
			red = intensity.val[2];

			//lưu điểm ảnh dưới dạng nhị phân
			rgbConversion = BgrToBinary(blue, green, red);

			/////////////////////////////////////////
			//Henon map: x_n+1=1-a*x^2_n+y_n
			//			 y_n+1=b*x_n
			parameters[3] = HenonMap();

			//isinf=true nếu giá trị là vô cùng
			if (isinf(parameters[3])) {
				cout << "Value diverged to infinity, exiting \n";
				cout << "Try again with different initial conditions \n";
				return;
			}

			// Tạo khóa dựa vào henon map
			memcpy(&u_2, &parameters[3], sizeof u_2);
			for (int i = 23; i >= 0; i--)
				LSB_2[i] = ('0' + ((u_2 >> i) & 1)) == '0' ? 0 : 1;

			//XOR điểm ảnh với khóa
			encryptedPixel = rgbConversion ^ LSB_2;
			////////////////////////////////////////////

			//trả về mảng BGR sau khi XOR với khóa
			BGR = extractBGR(encryptedPixel);

			//Lưu lại từng B G R sau khi mã 
			//Blue 
			intensity.val[0] = BGR[0];
			//Green
			intensity.val[1] = BGR[1];
			//Red
			intensity.val[2] = BGR[2];

			//Lưu lại từng điểm ảnh sau khi encrypt
			img.at<Vec3b>(i, j) = intensity;

		}
	}

	//Hiển thị ảnh đã mã hóa
	cv::imshow("Diffusion", img);
	cv::moveWindow("Diffusion", 40, 30);
	
	return;
}
//Thay thế dựa theo LogisticMap
void Sustitution(Mat& img)
{
	//khai báo biến
	int c = 0;
	int r = 0;
	x.size();
	y.size();
	//thay thế từng pixel ảnh dựa vào z
	for (int i = 0; i < img.rows; ++i)
	{
		for (int j = 0; j < img.cols; ++j)
		{
			if (c >= img.cols)
			{
				c = 0;
			}
			if (r >= img.rows)
			{
				r = 0;
			}
			//tem_c là stt của phần tử đầu trong vecto x
			int temp_c = x[c].second;
			//tem_r là stt của phần tử đầu trong vecto y
			int temp_r = y[r].second;
			//Thay thế các pixel ảnh
			pixel = img.at<Vec3b>(temp_r, temp_c);
			img.at<Vec3b>(temp_r, temp_c) = img.at<Vec3b>(i, j);
			img.at<Vec3b>(i, j) = pixel;
			c++;
		}
		r++;
	}
	imshow("Encrypted", img);
	//Lưu ảnh đã mã hóa vào outputPath
	cv::imwrite(outputPath, img);
}
#pragma endregion Encrypt

#pragma region Decrypt
//Thay thế dựa theo LogisticMap
void InvSustitution(Mat& img)
{
	imshow("Input image", img);
	double temp;
	for (int i = 0; i < img.cols; ++i)
	{
		//Dựa vào logistic tạo chuỗi ngẫu nhiên
		temp = parameters[1] = parameters[0] * parameters[1] * (1 - parameters[1]);
		x.push_back({ temp,i });

	}
	// Tạo Y seq ngẫu nhiên dưa vào Logistic map với số phần tử bằng chiều cao ảnh
	for (int i = 0; i < img.rows; ++i)
	{
		//Dựa vào logistic tạo chuỗi ngẫu nhiên
		temp = parameters[1] = parameters[0] * parameters[1] * (1 - parameters[1]);
		y.push_back({ temp,i });

	}
	//Sắp xếp giá trị không giảm
	std::sort(x.begin(), x.end());
	std::sort(y.begin(), y.end());
	//khai báo biến
	int c = img.cols-1;
	int r = img.rows-1;
	//thay thế từng pixel ảnh dựa vào z
	for (int i = img.rows - 1; i >= 0; --i)
	{
		for (int j = img.cols - 1; j >= 0; --j)
		{
			if (c <0)
			{
				c = img.cols - 1;
			}
			if (r <0)
			{
				r = img.rows- 1;
			}
			//tem_c là stt của phần tử đầu trong vecto x
			int temp_c = x[c].second;
			//tem_r là stt của phần tử đầu trong vecto y
			int temp_r = y[r].second;
			//Thay thế các pixel ảnh
			pixel = img.at<Vec3b>(temp_r, temp_c);
			img.at<Vec3b>(temp_r, temp_c) = img.at<Vec3b>(i, j);
			img.at<Vec3b>(i, j) = pixel;
			c--;
		}
		r--;
	}
}
//Khuếch tán dựa theo HenonMap
void InvDiffusion(Mat& img)
{

	// Khai báo biến
	bitset<24> LSB; //key 1 logistic
	bitset<24> LSB_2;//key 2 henon
	bitset<24> encryptedPixel;//điểm ảnh
	bitset<24> originalPixel;//Ảnh sau giải mã
	bitset<24> xorVal;

	array<int, 3> BGR;

	int blue;
	int green;
	int red;

	uint64_t u_2;

	//Xét từng pixel của ảnh
	for (int i = 0; i < img.rows; i++)
	{
		for (int j = 0; j < img.cols; j++)
		{
			//lưu ảnh theo BGR
			Vec3b intensity = img.at<Vec3b>(i, j);
			blue = intensity.val[0];
			green = intensity.val[1];
			red = intensity.val[2];

			//lưu điểm ảnh dưới dạng nhị phân
			encryptedPixel = BgrToBinary(blue, green, red);

			//Henon map: x_n+1=1-a*x^2_n+y_n
			//			 y_n+1=b*x_n
			parameters[3] = HenonMap();

			//isinf=true nếu giá trị là vô cùng
			if (isinf(parameters[3])) {
				cout << "Value diverged to infinity, exiting \n";
				cout << "Try again with different initial conditions \n";
				return;
			}

			// Tạo khóa dựa vào henon map
			memcpy(&u_2, &parameters[3], sizeof u_2);
			for (int i = 23; i >= 0; i--)
				LSB_2[i] = ('0' + ((u_2 >> i) & 1)) == '0' ? 0 : 1;

			//OR khóa và điểm ảnh
			originalPixel = LSB_2 ^ encryptedPixel;

			//isinf=true nếu giá trị là vô cùng
			if (isinf(parameters[1])) {
				cout << "Value diverged to infinity, exiting \n";
				cout << "Try again with different initial conditions \n";
				return;
			}

			BGR = extractBGR(originalPixel);

			//Blue 
			intensity.val[0] = BGR[0];
			//Green
			intensity.val[1] = BGR[1];
			//Red
			intensity.val[2] = BGR[2];

			//Lưu lại từng điểm ảnh sau khi giải mã
			img.at<Vec3b>(i, j) = intensity;


		}
	}
}
//Hoán vị dựa theo logistic map
void InvPermutation(Mat& img)
{
	//Hoán vị ảnh dựa vào y
	int v = img.rows - 1;
	for (int i = img.rows - 1; i >= 0; --i)
	{
		for (int j = img.cols - 1; j >= 0; --j)
		{
			if (v < 0)
				v = img.rows - 1;
			int temp = y[v].second;
			pixel = img.at<Vec3b>(temp, j);
			img.at<Vec3b>(temp, j) = img.at<Vec3b>(i, j);
			img.at<Vec3b>(i, j) = pixel;
			v--;
		}
	}
	v = img.cols - 1;
	for (int i = img.rows - 1; i >= 0; --i)
	{
		for (int j = img.cols - 1; j >= 0; --j)
		{
			if (v < 0)
				v = img.cols - 1;
			int temp = x[v].second;
			pixel = img.at<Vec3b>(i,temp);
			img.at<Vec3b>(i, temp) = img.at<Vec3b>(i, j);
			img.at<Vec3b>(i, j) = pixel;
			v--;
		}
	}
	//Hiển thị ảnh sau khi hoán vị
	cv::imshow("Original image", img);
	cv::moveWindow("Original image", 40, 30);
}
#pragma endregion Decrypt

#pragma region Input
void mapParamsInput()
{


	cout << endl << "------------------------------------------------- \n";
	cout << "   Logistic map encryption \n \n";
	cout << "   x_{n+1} = r (1- x_n) \n";
	cout << "------------------------------------------------- \n \n";

	string inputs[] = { "r (Growth rate) : " , "x_n (Initial condition) : " };
	bool failed = true;

	for (size_t i = 0; i < 2; i++)
	{
		do {
			cin.clear();
			cout << inputs[i];

			cin >> parameters[i];

			if (cin.fail()) {
				cout << "Invalid input, please enter a number" << endl << "\n";
				cin.clear();
				cin.ignore(numeric_limits<streamsize>::max(), '\n');
				failed = true;
			}
			else {
				failed = false;
			}

		} while (failed);

		cout << "\n";
	}
	cout << "   Henon map encryption \n \n";
	cout << "   x_{n+1} = 1 - (a - (x_n)^2) + y_n \n";
	cout << "   y_{n+1} = b * x_n \n";
	cout << "------------------------------------------------- \n \n"; \
		string inputs2[] = { "y : " , "x : " , "a : " , "b : " };
	failed = true;

	for (size_t i = 0; i < 4; i++)
	{
		do {
			cin.clear();
			cout << inputs2[i];

			cin >> parameters[i + 2];

			if (cin.fail()) {
				cout << "Invalid input, please enter a number" << endl << "\n";
				cin.clear();
				cin.ignore(numeric_limits<streamsize>::max(), '\n');
				failed = true;
			}
			else {
				failed = false;
			}

		} while (failed);

		cout << "\n";
	}
}



void showWelcomeScreen() {

	mapParamsInput();;

	cout << "Opeartions : (1) Encrypt or (2) Decrypt " << endl;

	do {
		cin.clear();
		cout << "Enter operation number : ";

		cin >> operation;

		if (cin.fail() || !(operation == 1 || operation == 2)) {
			cout << "Invalid input, please enter either 1 or 2" << endl << "\n";
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
		}

	} while (!(operation == 1 || operation == 2));

	bool isImage = false;

	ifstream ifile;
	string operationStr = (operation == 1 ? "encrypt" : "decrypt");

	while (!ifile || !isImage)
	{
		cout << "path of image to " << operationStr << " : ";
		//nhập link ảnh cần mã hóa hay giải mã
		getline(cin >> ws, imagePath);


		ifile.open(imagePath);

		if (!ifile) {
			cout << "file doesn't exist, try again" << endl << "\n";
		}
		else {
			string extn = imagePath.substr(imagePath.find_last_of(".") + 1);

			if (extn == "png" || extn == "jpg") {
				isImage = true;
				cout << operationStr << "ed image will be written to the same folder" << endl << "\n";
			}
			else {
				cout << "file chosen is not an image, try again" << endl << "\n";
				isImage = false;
			}
		}

		ifile.close();
	}
	size_t found;
	found = imagePath.find_last_of(".");
	outputPath = imagePath;
	outputPath.replace(found, sizeof(outputPath), "_" + operationStr + "ed.png");
	;
}
#pragma endregion Input

#pragma region Compress
void CompressImg()
{

	char filePath[200];
	char savePath[200];
	cout << "Perform image compression!!!" << endl;
	cout << "path of image to compress: ";
	//nhập link ảnh cần nén
	cin.ignore();
	cin.getline(filePath, 200);
	cout << "path of image to save: ";
	cin.getline(savePath, 200);
	Compressor c;
	c.modFunc(filePath, savePath);
}
#pragma endregion Compress
#pragma region Histogram
void Histogram(Mat src)
{
	vector<Mat> bgr_planes;
	split(src, bgr_planes);
	int histSize = 256;
	float range[] = { 0, 256 }; //the upper boundary is exclusive
	const float* histRange[] = { range };
	bool uniform = true, accumulate = false;
	Mat b_hist, g_hist, r_hist;
	calcHist(&bgr_planes[0], 1, 0, Mat(), b_hist, 1, &histSize, histRange, uniform, accumulate);
	calcHist(&bgr_planes[1], 1, 0, Mat(), g_hist, 1, &histSize, histRange, uniform, accumulate);
	calcHist(&bgr_planes[2], 1, 0, Mat(), r_hist, 1, &histSize, histRange, uniform, accumulate);
	int hist_w = 512, hist_h = 400;
	int bin_w = cvRound((double)hist_w / histSize);
	Mat histImage(hist_h, hist_w, CV_8UC3, Scalar(0, 0, 0));
	normalize(b_hist, b_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());
	normalize(g_hist, g_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());
	normalize(r_hist, r_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat());
	for (int i = 1; i < histSize; i++)
	{
		line(histImage, Point(bin_w * (i - 1), hist_h - cvRound(b_hist.at<float>(i - 1))),
			Point(bin_w * (i), hist_h - cvRound(b_hist.at<float>(i))),
			Scalar(255, 0, 0), 2, 8, 0);
		line(histImage, Point(bin_w * (i - 1), hist_h - cvRound(g_hist.at<float>(i - 1))),
			Point(bin_w * (i), hist_h - cvRound(g_hist.at<float>(i))),
			Scalar(0, 255, 0), 2, 8, 0);
		line(histImage, Point(bin_w * (i - 1), hist_h - cvRound(r_hist.at<float>(i - 1))),
			Point(bin_w * (i), hist_h - cvRound(r_hist.at<float>(i))),
			Scalar(0, 0, 255), 2, 8, 0);
	}
	imshow("calcHist Demo", histImage);
	waitKey();
}
#pragma endregion 


int main()
{
	////////////////////////////////////////////////////////////////////////////
	//				COMPRESSION
	cout << "Do you want to compress image?  (1) Yes or (2) No" << endl;
	int n;
	do {
		cin.clear();
		cout << "Enter operation number : ";

		cin >> n;

		if (cin.fail() || !(n == 1 || n == 2)) {
			cout << "Invalid input, please enter either 1 or 2" << endl << "\n";
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
		}

	} while (!(n == 1 || n == 2));
	if (n == 1)
	{
		CompressImg();
	}
	//
	////////////////////////////////////////////////////////////////////////////////
	//				 ENCRYPT OR DECRYPT
	
	showWelcomeScreen();
	if (operation == 1)
	{
		Mat img = imread(imagePath);
		cout << "Encrypting : " << imagePath << "\n" << endl;
		//Histogram(img);
		Permutation(img);
		Diffusion(img);
		Sustitution(img);
		Histogram(img);
		cout << "Encryption finished successfully \n";
		//Lưu ảnh đã mã hóa vào outputPath
		cv::imwrite(outputPath, img);
	}
	else if (operation == 2)
	{
		Mat img = imread(imagePath);
		cout << "Decrypting : " << imagePath << "\n" << endl;
		InvSustitution(img);
		InvDiffusion(img);
		InvPermutation(img);
		cout << "Decryption finished successfully \n";
		//Lưu ảnh đã mã hóa vào outputPath
		cv::imwrite(outputPath, img);
	}

	waitKey(0);
	
	std::system("pause");
	return 0;

}