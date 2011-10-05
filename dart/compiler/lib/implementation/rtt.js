// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

// The following methods are used to handle type information
//

/** 
 * @constructor
 * @param {string} classkey
 * @param {string=} typekey
 * @param {Array.<RTT>=} typeargs
 */ 
function RTT(classkey, typekey, typeargs) {
  this.classKey = classkey;
  this.typeKey = typekey ? typekey : classkey;
  this.typeArgs = typeargs;
  this.implementedTypes = {};
  // Add self
  this.implementedTypes[classkey] = this;
  // Add Object
  if (classkey != 'Object') {
    this.implementedTypes['Object'] = RTT.objectType;
  }
}

/** @type {Object.<string, Object>} */
RTT.types = {};

/** @type {Array.<RTT>} */
RTT.prototype.derivedTypes = [];

/** @return {string} */
RTT.prototype.toString = function() { return this.typeKey; }

/**
 * @param {*} value 
 * @return {boolean} Whether this type is implemented by the value
 */
RTT.prototype.implementedBy = function(value){ 
  return (value == null) ? RTT.nullInstanceOf(this) :
      this.implementedByType(RTT.getTypeInfo(value)); 
};

/**
 * @param {!RTT} other
 * @return {boolean} Whether this type is implement by other
 */
RTT.prototype.implementedByType = function(otherType) { 
  if (otherType === this || otherType === RTT.dynamicType) {
    return true;
  }
  var targetTypeInfo = otherType.implementedTypes[this.classKey];
  if (targetTypeInfo == null) { 
    return false; 
  }
  if (targetTypeInfo.typeArgs && this.typeArgs) {
    for(var i = this.typeArgs.length - 1; i >= 0; i--) {
      if (!this.typeArgs[i].implementedByType(targetTypeInfo.typeArgs[i])) {
        return false;
      }
    }
  }
  return true;
};

/** 
 * @param {RTT}
 * @return {boolean} 
 */
RTT.nullInstanceOf = function(type) {
  return type === RTT.objectType || type === RTT.dynamicType;
};

/**
 * @param {*} value The value to retrieve type information for
 * @return {RTT} 
 */
RTT.getNativeTypeInfo = function(value) {
  if (value instanceof Array) return Array.$lookupRTT();
  switch (typeof value) {
    case 'string': return String.$lookupRTT();
    case 'number': return Number.$lookupRTT();
    case 'boolean': return Boolean.$lookupRTT();
  } 
  return RTT.placeholderType;
};

/**
 * @param {string} name
 * @param {function(RTT,Array.<RTT>)=} implementsSupplier 
 * @param {Array.<RTT>=} typeArgs
 * @return {RTT} The RTT information object
 */
RTT.create = function(name, implementsSupplier, typeArgs) {
  if (name == "Object") return RTT.objectType;
  var typekey = RTT.getTypeKey(name, typeArgs);
  var rtt = RTT.types[typekey];
  if (rtt) {
    return rtt;
  }
  var classkey = RTT.getTypeKey(name);
  rtt = new RTT(classkey, typekey, typeArgs);
  RTT.types[typekey] = rtt;
  if (implementsSupplier) {
    implementsSupplier(rtt, typeArgs);
  }
  return rtt;
};

/**
 * @param {string} classkey
 * @param {Array.<(RTT|string)>=} typeargs
 * @return {string}
 */
RTT.getTypeKey = function(classkey, typeargs) {
  var key = classkey;
  if (typeargs) {
    key += "<" + typeargs.join(",") + ">";
  }
  return key;
};

/**
 * @return {*} value
 * @return {RTT} return the RTT information object for the value
 */
RTT.getTypeInfo = function(value) {
  return (value.$typeInfo) ? value.$typeInfo : RTT.getNativeTypeInfo(value);
};

/**
 * @param {Object} o
 * @param {RTT} rtt
 * Sets the RTT on the object and returns the object itself.
 */
RTT.setTypeInfo = function(o, rtt) {
  o.$typeInfo = rtt;
  return o;
};

/**
 * @param {Object} o
 * Removes any RTT from the object and returns the object itself.
 */
RTT.removeTypeInfo = function(o) {
  o.$typeInfo = null;
  return o;
};

/**
 * The typeArg array is optional
 * @param {Array.<RTT>=} typeArgs
 * @param {number} i
 * @return {RTT}
 */
RTT.getTypeArg = function(typeArgs,i) {
  if (typeArgs) {
    if (typeArgs.length > i) {
      return typeArgs[i];
    } else {
      throw new Error("Missing type arg");
    }
  } 
  return RTT.dynamicType; 
};

/**
 * The typeArg array is optional
 * @param {*} o
 * @param {string} classkey
 * @param {number} i
 * @return {Array.<RTT>}
 */
RTT.getTypeArgsFor = function(o, classkey) {
  var rtt = RTT.getTypeInfo(o).implementedTypes[classkey];
  if (!rtt) {
    throw new Error("internal error: can not find " + classkey + " in " + JSON.stringify(o));
  }
  return rtt.typeArgs;
};

// Base types for runtime type information

/** @type {!RTT} */
RTT.objectType = new RTT('Object');
RTT.objectType.implementedBy = function(o) {return true};
RTT.objectType.implementedByType = function(o) {return true};

/** @type {!RTT} */
RTT.dynamicType = new RTT('Dynamic');
RTT.dynamicType.implementedBy = function(o) {return true};
RTT.dynamicType.implementedByType = function(o) {return true};

/** @type {!RTT} */
RTT.placeholderType = new RTT('');
RTT.placeholderType.implementedBy = function(o) {return true};
RTT.placeholderType.implementedByType = function(o) {return true};

