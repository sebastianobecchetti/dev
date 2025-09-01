classdef class1
    properties (Access = private)
        a 
        b
    end
    methods
        function obj = class1(x,y)
            obj.a = x+y;
            obj.b = x./y;
        end
        function plot(obj)
            figure(1)
            plot(obj.b)
        end
    end
end