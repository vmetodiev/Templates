#include <stdio.h>
#include <stdint.h>

//
// Serializer functions
//
void serialize_uint16(uint16_t var, uint8_t *buffer);
void serialize_uint32(uint32_t var, uint8_t *buffer);

// Note: the approach is more difficult (with pointer typecasting) but could 
// be ported to  literally any other type (the example is only with "float")
void serialize_float(float var, uint8_t *buffer);


//
// De-serializer functions
//
uint16_t deserialize_uint16(uint8_t *buffer);
uint32_t deserialize_uint32(uint8_t *buffer);
float deserialize_float(uint8_t *buffer);

//
// Dummy way to serialize uint16 (you can port it any other 2-byte type)
// 
void serialize_uint16(uint16_t var, uint8_t *buffer)
{
    const size_t size_in_bytes = 2;
    
    for ( size_t n = 0; n < size_in_bytes; n++ )
    {    
        buffer[n] = 0x00;
        buffer[n] = (var >> (8 * n) ) & 0xFF;

        // printf("var[%lu] = %u\n",n , buffer[n] );
    }
    return;
}

//
// Dummy way to serialize uint32 (you can port it any other 4-byte type, maybe "int") 
// 
void serialize_uint32(uint32_t var, uint8_t *buffer)
{
    const size_t size_in_bytes = 4;

    for ( size_t n = 0; n < size_in_bytes; n++ )
    {    
        buffer[n] = 0x00;
        buffer[n] = (var >> (8 * n) ) & 0xFF;
    }
    return;
}

//
// Smarter way to serialize any type (example with float)
//
void serialize_float(float var, uint8_t *buffer)
{
    const size_t size_in_bytes = 4;
    // Add an assert that the "float" is 4 bytes on your system
    
    uint8_t *var_ptr = (uint8_t *)&var;

    for ( size_t n = 0; n < size_in_bytes; n++ )
    {    
        buffer[n] = 0x00;
        buffer[n] = var_ptr[n] & 0xFF;
    }
    return;
}


uint16_t deserialize_uint16(uint8_t *buffer)
{
    uint16_t deserialized = *( (uint16_t *)buffer );
    return deserialized;
}

uint32_t deserialize_uint32(uint8_t *buffer)
{
    uint32_t deserialized = *( (uint32_t *)buffer );
    return deserialized;
}

float deserialize_float(uint8_t *buffer)
{
    float deserialized = *( (float *)buffer );
    return deserialized;
}

int main()
{
    uint8_t my_var_byte_arr[64]; // Allocate buffer on the stack, enough even for 512-bit variables
    
    //
    // uint16 example
    //
    uint16_t my_uint16_var = 260;
    uint16_t my_uint16_deser = 0;
    
    serialize_uint16(my_uint16_var, my_var_byte_arr);
    my_uint16_deser = deserialize_uint16(my_var_byte_arr);
    printf("Example 1 | my_uint16_deser = %u\n", my_uint16_deser);

    //
    // uint32 example
    //
    uint32_t my_uint32_var = 78123;
    uint32_t my_uint32_deser = 0;
    
    serialize_uint32(my_uint32_var, my_var_byte_arr);
    my_uint32_deser = deserialize_uint32(my_var_byte_arr);
    printf("Example 2 | my_uint32_deser = %u\n", my_uint32_deser);

    //
    // float example
    //
    float my_float_var = -25.26;
    float my_float_deser = 0;
    
    serialize_float(my_float_var, my_var_byte_arr);
    my_float_deser = deserialize_float(my_var_byte_arr);
    printf("Example 3 | my_float_deser = %f\n", my_float_deser);

    return 0;
}