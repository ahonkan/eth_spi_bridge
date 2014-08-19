require 'rbconfig'

class ContextSerializerTransform < Transform
   def initialize(phaseList)
   end
   
   def executePhase(context)
      @context = context
      puts "This is where we'd serialize context"
      if true
        serialize("universe.data")

        deserialize("universe.data")
      end  

      return true

   end
   
   def serialize(filename)
      # Write to disk with FileOutputStream
      f_out = Java::JavaIo::FileOutputStream.new(filename)

      # Write object with ObjectOutputStream
      obj_out = Java::JavaIo::ObjectOutputStream.new(f_out)

      # Write object out to disk
      obj_out.writeObject(@context)
      f_out.close()
   
   end

   def deserialize(filename)
      f_in = Java::JavaIo::FileInputStream.new(filename)
      puts f_in.class
      if f_in == nil
        puts "Error reading file"
        raise Exception,"Error reading file"
      end  
      
      obj_in = Java::JavaIo::ObjectInputStream.new(f_in)
      if obj_in == nil
        puts "Error reading object"
        raise Exception,"Error reading serialized object"
      end  
      
      new_context = obj_in.readObject()
      
      @context = new_context
   
   end
end


