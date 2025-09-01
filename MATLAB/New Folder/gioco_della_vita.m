clear,clc

N = 30;
A = zeros([N,N]);
A([304 305 306]) = 1;    % esempio: riga 10, colonne 4-6
%A([304 334 364 363 332]) = 1; 



numgen = 100;


for ii = 1:100
    B = zeros([N,N]);
    C = conv2(A,[1 1 1;1 0 1;1 1 1], 'same');
    B(C==3 & A==1) = 1;
    B(C==2 & A==1) = 1;
    B(C>=3 & A==0) = 1;
    spy(B)
    drawnow
    A = B;
end
