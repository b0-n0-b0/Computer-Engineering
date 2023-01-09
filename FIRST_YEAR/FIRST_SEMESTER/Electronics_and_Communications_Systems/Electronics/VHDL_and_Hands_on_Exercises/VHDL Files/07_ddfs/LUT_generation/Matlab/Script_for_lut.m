%%
%Script to generate the DDFS Look-Up Table
%%

LUT_INTEGER_FLAG = 1; %PUT THIS FLAG TO 1 IF YOU WANT TO GENERATE THE LOOP BY MEANS OF INTEGERS.

N = 12; %Number of bit for the phase quantization.
lsb_phase = 2*pi /(2^N); %phase lsb.
phase = lsb_phase * (0 : 2^N-1); %phase values
h = sin(phase); %sine values. 
%h is not ready to be contained into the loop. 
%An amplitude quantization is also necessary.
M = 6; %Number of bit for the amplitude quantization.
lsb_amplitude = 1 / (2^(M-1) - 1);%amplitude lsb (Using a balanced C2 representation)
h_q = round(h/lsb_amplitude); %Quantized sine values.

if(LUT_INTEGER_FLAG)
    plot(h_q); %it plots h_q 
    grid on;% it shows the grid
    fileID = fopen('int_lut.txt','w'); %It opens a file named 'int_lut.txt' to contain the data.
    fprintf(fileID, '%d,\n',h_q(1 : 2^N-1)); %It writes the integer values separated by ",\n".
    fprintf(fileID, '%d\n',h_q(2^N));%It writes the last integer value separated by "\n".
    fclose(fileID); %It closes the file.
else
    plot(h_q,'r');%it plots h_q using the red color.
    hold on %It prevents the plot from disappearing.
    h_q(h_q < 0) = h_q(h_q < 0) + 2^M; %2-complement representation since the dec2bin does not accept negative value;
    plot(h_q,'b');%it plots the 2-complemented h_q using the blue color.
    grid on; %It shows the grid.
    fileID = fopen('binary_lut.txt','w');%It opens a file named 'binary_lut.txt' to contain the data.
   
    for i = 1 : 2^N - 1
         fprintf(fileID,'"%s",\n',  dec2bin(h_q(i),M)); %Each value is converted in binary notation, using a M-bit representation, and printed to the file, separed by ',\n' and between "".
    end
         fprintf(fileID,'"%s"\n',  dec2bin(h_q(2^N),M)); %The last value is converted in binary, using a M-bit representation,and printed to the file separed by '\n' and between "".
  
    fclose(fileID);
    
end

