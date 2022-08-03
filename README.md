A LIGHTWEIGHT AND EFFICIENT DIGITAL IMAGE ENCRYPTION USING HYBRID CHAOTIC SYSTEMS.
The project proposes a digital image encoding method based on DCT image compression and a combination of Logistic Map and Henon Map. The purpose of image compression is to reduce their size, increase the encoding speed and break the correlation between the image pixel values.  In this image encryption, the chaos map provides mainly a sequence of random numbers used as the key, and the random image is shuffled through two ciphertext generation processes. During the encryption and decryption process, the key used in the digital image encryption method is the computational parameter of the Logistic and Henon maps.

LIBRARY NEEDED:
- OpenCV

Run file Encryption.cpp to use

Steps to Compress

- Input full image file path to compress
- Input full image file path to save

Steps to Encrypt

- Input the parameters of Logistic Map, the values of interest for the parameter r  are those in the interval [0.4], so that xn remains bounded on [0,1].
- Input the parameters of Henon Map
	+ The Hénon map maps two points into themselves: these are the invariant points. For the classical values of a and b of the Hénon map, one of these points is on the attractor:
		x=0.631354477
		y=0.189406343
	+ The map depends on two parameters, a and b, which for the classical Hénon map have values of a = 1.4 and b = 0.3
- Input full image file path to encrypt
- Encrypted image will be written to the same folder

Steps to Decrypt: like Encrypt

Study References

-https://www.sciencedirect.com/science/article/abs/pii/S2214212618307488

Implementation References

-https://github.com/rezeck/CompressorJPEG

Result :
original image
<img src="https://user-images.githubusercontent.com/90474684/182527402-2a522bec-bb94-4869-ac14-898ef92258fe.png" alt="original image" />
After using Logistic Map 1
<img src="https://user-images.githubusercontent.com/90474684/182527583-9bb97550-44d8-4a12-9875-8557eca6dd81.png" alt="Logistic Map 1" />
After using Henon Map
<img src="https://user-images.githubusercontent.com/90474684/182527700-1c20414a-daa3-4e91-be84-d22176ab4b00.png" alt="Henon Map" />
Final
<img src="https://user-images.githubusercontent.com/90474684/182527787-2827b79c-cca8-46df-9575-249981d66672.png" alt="Final" />
Decrypted image
<img src="https://user-images.githubusercontent.com/90474684/182527913-b462f22c-8a62-45f8-ae66-4371d5318560.png" alt="Decrypted image" />
