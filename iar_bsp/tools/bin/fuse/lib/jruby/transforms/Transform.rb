class Transform < Java::ComMentorNucleusFusePhases::Phase
   include Java::ComMentorNucleusFuseCoreVisitors::Visitor

   FuseType = Java::ComMentorNucleusFuseCoreTypes
   @@class_prefix = "Java::ComMentorNucleusFuseCoreTypes::"
   
   def preVisit(obj)
      if anointed?(obj)
         self.visit("preVisit", obj)
      end
   end
   
   def postVisit(obj)
      if anointed?(obj)
         self.visit("postVisit", obj)
      end
   end

   def visit(visit_name, obj)
      if obj.class.name.index(@@class_prefix)
        method_name = visit_name + obj.class.name.gsub(@@class_prefix, "")
        if self.methods.member?(method_name)
          self.send(method_name, obj)
        end
      end
   end

   def error(msg)
   end

   def warn(msg)
   end

   def anointed?(obj)
      obj.hasProperty("anointed") and obj.getBooleanProperty("anointed")
   end
end
